#include "motor_controller.h"
#include <iostream> // For m_lastError printing, replace with QDebug or logging
#include <iomanip>  // For std::hex

// Helper to convert int types to byte vectors for SDO
template<typename T>
std::vector<unsigned char> toBytes(T value) {
    std::vector<unsigned char> bytes(sizeof(T));
    const unsigned char* p = reinterpret_cast<const unsigned char*>(&value);
    for (size_t i = 0; i < sizeof(T); ++i) {
        bytes[i] = p[i];
    }
    return bytes;
}

// Helper to convert byte vectors from SDO to int types
template<typename T>
T fromBytes(const std::vector<unsigned char>& bytes) {
    if (bytes.size() < sizeof(T)) {
        // Handle error: not enough bytes. Perhaps throw or return a default.
        // For now, print error and return 0.
        std::cerr << "Error: fromBytes - not enough bytes to convert to type. Expected "
                  << sizeof(T) << ", got " << bytes.size() << std::endl;
        return T{0};
    }
    T value;
    unsigned char* p = reinterpret_cast<unsigned char*>(&value);
    for (size_t i = 0; i < sizeof(T); ++i) {
        p[i] = bytes[i];
    }
    return value;
}


MotorController::MotorController(CANHandler& canHandler, uint16_t nodeId)
    : m_canHandler(canHandler), m_nodeId(nodeId) {}

MotorController::~MotorController() {}

bool MotorController::writeSDO(uint16_t index, uint8_t subIndex, const std::vector<unsigned char>& data) {
    unsigned char sdoResponseCs = 0;
    if (!m_canHandler.sendSdoWriteRequest(m_nodeId, index, subIndex, data)) {
        m_lastError = "Failed to send SDO Write Request: " + m_canHandler.getLastError();
        return false;
    }
    // Wait for confirmation
    std::vector<unsigned char> receivedData; // Should be empty for write confirmation
    if (!m_canHandler.receiveSdoResponse(m_nodeId, index, subIndex, 100, receivedData, sdoResponseCs)) {
        m_lastError = "Failed to receive SDO Write Confirmation: " + m_canHandler.getLastError();
        return false;
    }
    if (sdoResponseCs == 0x80) { // SDO Abort
        // Error already set by CANHandler, but we can augment it if needed
        m_lastError = "SDO Write Aborted for " + std::to_string(index) + ":" + std::to_string(subIndex) + ". " + m_canHandler.getLastError();
        return false;
    }
    if (sdoResponseCs != 0x60) { // Not a successful download response
        m_lastError = "Unexpected SDO response CS: 0x" + std::to_string(sdoResponseCs) + " for write to " + std::to_string(index) + ":" + std::to_string(subIndex);
        return false;
    }
    m_lastError = "SDO Write successful for " + std::to_string(index) + ":" + std::to_string(subIndex);
    return true;
}

bool MotorController::readSDO(uint16_t index, uint8_t subIndex, uint32_t timeoutMs, std::vector<unsigned char>& receivedData) {
    unsigned char sdoResponseCs = 0;
    if (!m_canHandler.sendSdoReadRequest(m_nodeId, index, subIndex)) {
        m_lastError = "Failed to send SDO Read Request: " + m_canHandler.getLastError();
        return false;
    }

    if (!m_canHandler.receiveSdoResponse(m_nodeId, index, subIndex, timeoutMs, receivedData, sdoResponseCs)) {
        m_lastError = "Failed to receive SDO Read Response: " + m_canHandler.getLastError();
        return false;
    }

    if (sdoResponseCs == 0x80) { // SDO Abort
        m_lastError = "SDO Read Aborted for " + std::to_string(index) + ":" + std::to_string(subIndex) + ". " + m_canHandler.getLastError();
        return false;
    }

    // Check if it's an upload response (scs bits 010)
    if ((sdoResponseCs >> 5) != 0x02) {
        m_lastError = "Unexpected SDO response CS: 0x" + std::to_string(sdoResponseCs) + " for read from " + std::to_string(index) + ":" + std::to_string(subIndex);
        return false;
    }

    // Data is already in receivedData by CANHandler
    m_lastError = "SDO Read successful for " + std::to_string(index) + ":" + std::to_string(subIndex);
    return true;
}


bool MotorController::setModeOfOperation(int8_t mode) {
    std::vector<unsigned char> data = toBytes(mode);
    if (!writeSDO(MotorSdoObjects::MODES_OF_OPERATION_INDEX, MotorSdoObjects::MODES_OF_OPERATION_SUBINDEX, data)) {
        m_lastError = "Failed to set Mode of Operation: " + m_lastError; // m_lastError already set by writeSDO
        return false;
    }
    m_lastError = "Mode of Operation set to " + std::to_string(mode);
    return true;
}

bool MotorController::readStatusword(uint16_t& statusword) {
    std::vector<unsigned char> receivedData;
    if (!readSDO(MotorSdoObjects::STATUSWORD_INDEX, MotorSdoObjects::STATUSWORD_SUBINDEX, 100, receivedData)) {
        m_lastError = "Failed to read Statusword: " + m_lastError; // m_lastError already set by readSDO
        statusword = 0;
        return false;
    }
    if (receivedData.size() < sizeof(uint16_t)) {
        m_lastError = "Statusword SDO read returned insufficient data. Expected " + std::to_string(sizeof(uint16_t)) + " bytes, got " + std::to_string(receivedData.size());
        statusword = 0;
        return false;
    }
    statusword = fromBytes<uint16_t>(receivedData);
    return true;
}

// Simplified CANopen state machine transitions to enable motor
// Based on CiA 402 state machine:
// Shutdown (0x06) -> Switch On (0x07) -> Operation Enable (0x0F)
bool MotorController::enableMotor() {
    uint16_t status = 0;
    // Read Statusword first to check current state (optional, good practice)
    if (!readStatusword(status)) {
        m_lastError = "EnableMotor: Failed to read statusword first. " + m_lastError;
        return false;
    }
    std::cout << "Initial Statusword: 0x" << std::hex << status << std::dec << std::endl;

    // To transition from "Fault" or "Switch On Disabled"
    // Send Controlword 0x0080 (Fault Reset, if applicable, not always needed from Switch On Disabled)
    // Here, we assume we are in "Switch On Disabled" or can reach "Ready to Switch On"

    // 1. Transition to "Ready to Switch On" (from "Switch On Disabled") - Controlword = 0x0006
    if (!writeSDO(MotorSdoObjects::CONTROLWORD_INDEX, MotorSdoObjects::CONTROLWORD_SUBINDEX, toBytes<uint16_t>(0x0006))) {
        m_lastError = "EnableMotor: Failed to send SHUTDOWN (0x06) to Controlword. " + m_lastError;
        return false;
    }
    std::cout << "Sent Controlword SHUTDOWN (0x0006)" << std::endl;
    // TODO: Add delay and check Statusword for (xxxx x0xx x01x xx0x) - "Ready to Switch On"

    // 2. Transition to "Switched On" (from "Ready to Switch On") - Controlword = 0x0007
    if (!writeSDO(MotorSdoObjects::CONTROLWORD_INDEX, MotorSdoObjects::CONTROLWORD_SUBINDEX, toBytes<uint16_t>(0x0007))) {
        m_lastError = "EnableMotor: Failed to send SWITCH ON (0x07) to Controlword. " + m_lastError;
        return false;
    }
    std::cout << "Sent Controlword SWITCH ON (0x0007)" << std::endl;
    // TODO: Add delay and check Statusword for (xxxx x0xx x01x x01x) - "Switched On"

    // 3. Transition to "Operation Enabled" (from "Switched On") - Controlword = 0x000F
    if (!writeSDO(MotorSdoObjects::CONTROLWORD_INDEX, MotorSdoObjects::CONTROLWORD_SUBINDEX, toBytes<uint16_t>(0x000F))) {
        m_lastError = "EnableMotor: Failed to send ENABLE OPERATION (0x0F) to Controlword. " + m_lastError;
        return false;
    }
    std::cout << "Sent Controlword ENABLE OPERATION (0x000F)" << std::endl;
    // TODO: Add delay and check Statusword for (xxxx x0xx x01x x11x) - "Operation Enabled"

    m_lastError = "Motor enable sequence sent.";
    // In a real application, verify Statusword confirms "Operation Enabled" state.
    return true;
}

bool MotorController::disableMotor() {
    // Transition to "Switched On" from "Operation Enabled" - Controlword = 0x0007
    // Or directly to "Ready to Switch On" - Controlword = 0x0006 (Shutdown)
    // Or "Switch On Disabled" - Controlword = 0x0000 (Disable Voltage)
    // Sending "Disable Voltage" (0x0000) is a common way to quickly disable.
    if (!writeSDO(MotorSdoObjects::CONTROLWORD_INDEX, MotorSdoObjects::CONTROLWORD_SUBINDEX, toBytes<uint16_t>(0x0000))) {
        m_lastError = "DisableMotor: Failed to send DISABLE VOLTAGE (0x00) to Controlword. " + m_lastError;
        return false;
    }
    m_lastError = "Motor disable sequence (Disable Voltage) sent.";
    return true;
}

bool MotorController::setTargetPosition(int32_t position) {
    // Ensure motor is in a mode that accepts Target Position (e.g., Profile Position Mode)
    // This should be set via setModeOfOperation(MotorSdoObjects::MODE_PROFILE_POSITION) first.
    // Also, typically, after setting a new target position, you might need to set bit 4 (new_set_point)
    // in the Controlword, or trigger the movement in another way depending on the drive's CiA 402 implementation.
    // For simplicity, just writing to the object.

    std::vector<unsigned char> data = toBytes(position);
    if (!writeSDO(MotorSdoObjects::TARGET_POSITION_INDEX, MotorSdoObjects::TARGET_POSITION_SUBINDEX, data)) {
        m_lastError = "Failed to set Target Position: " + m_lastError;
        return false;
    }

    // Optional: Trigger movement (example for some drives, might involve Controlword changes)
    // uint16_t controlWord = 0x001F; // Example: Set new set-point (bit 4) while keeping motor enabled (0x0F)
    // if (!writeSDO(MotorSdoObjects::CONTROLWORD_INDEX, MotorSdoObjects::CONTROLWORD_SUBINDEX, toBytes(controlWord))) {
    //     m_lastError = "Failed to set new_set_point in Controlword after Target Position. " + m_lastError;
    //     return false;
    // }
    // controlWord = 0x000F; // Clear new set-point (bit 4)
    // if (!writeSDO(MotorSdoObjects::CONTROLWORD_INDEX, MotorSdoObjects::CONTROLWORD_SUBINDEX, toBytes(controlWord))) {
    //    // ...
    // }

    m_lastError = "Target Position set to " + std::to_string(position);
    return true;
}

bool MotorController::readActualPosition(int32_t& position) {
    std::vector<unsigned char> receivedData;
    if (!readSDO(MotorSdoObjects::ACTUAL_POSITION_INDEX, MotorSdoObjects::ACTUAL_POSITION_SUBINDEX, 100, receivedData)) {
        m_lastError = "Failed to read Actual Position: " + m_lastError;
        position = 0;
        return false;
    }
    if (receivedData.size() < sizeof(int32_t)) {
        m_lastError = "Actual Position SDO read returned insufficient data. Expected " + std::to_string(sizeof(int32_t)) + " bytes, got " + std::to_string(receivedData.size());
        position = 0;
        return false;
    }
    position = fromBytes<int32_t>(receivedData);
    return true;
}

void MotorController::pollMotorStatus() {
    int32_t actualPos = 0;
    uint16_t status = 0;

    if (readActualPosition(actualPos)) {
        std::cout << "Polled Actual Position: " << actualPos << std::endl;
    } else {
        std::cout << "Poll: Failed to read actual position. Error: " << getLastError() << std::endl;
    }

    if (readStatusword(status)) {
        std::cout << "Polled Statusword: 0x" << std::hex << status << std::dec << std::endl;
    } else {
        std::cout << "Poll: Failed to read statusword. Error: " << getLastError() << std::endl;
    }
}

std::string MotorController::getLastError() const {
    return m_lastError;
}

// sdoAbortCodeToString could be useful if CANHandler doesn't already provide detailed errors
// std::string MotorController::sdoAbortCodeToString(uint32_t abortCode) { ... }
