#include "motor_controller.h"
#include <iostream>
#include <iomanip>  // For std::hex
#include <sstream>  // For std::stringstream in pollMotorStatus

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
        std::cerr << "Error: fromBytes - not enough bytes. Expected "
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

void MotorController::logMessage(const std::string& message) {
    // Simple logger to std::cout for now. Could be replaced with a more sophisticated logger.
    std::cout << "[MotorCtrl Node " << m_nodeId << "] " << message << std::endl;
}

bool MotorController::writeSDO(uint16_t index, uint8_t subIndex, const std::vector<unsigned char>& data) {
    unsigned char sdoResponseCs = 0;
    if (!m_canHandler.sendSdoWriteRequest(m_nodeId, index, subIndex, data)) {
        m_lastError = "Failed to send SDO Write Request: " + m_canHandler.getLastError();
        logMessage("Error: " + m_lastError);
        return false;
    }
    std::vector<unsigned char> receivedData;
    if (!m_canHandler.receiveSdoResponse(m_nodeId, index, subIndex, 100, receivedData, sdoResponseCs)) {
        m_lastError = "Failed to receive SDO Write Confirmation: " + m_canHandler.getLastError();
        logMessage("Error: " + m_lastError);
        return false;
    }
    if (sdoResponseCs == 0x80) {
        m_lastError = "SDO Write Aborted for " + std::to_string(index) + ":" + std::to_string(subIndex) + ". " + m_canHandler.getLastError();
        logMessage("Error: " + m_lastError);
        return false;
    }
    if (sdoResponseCs != 0x60) {
        m_lastError = "Unexpected SDO response CS: 0x" + std::to_string(sdoResponseCs) + " for write to " + std::to_string(index) + ":" + std::to_string(subIndex);
        logMessage("Error: " + m_lastError);
        return false;
    }
    // m_lastError = "SDO Write successful for " + std::to_string(index) + ":" + std::to_string(subIndex); // Success logged by caller if needed
    return true;
}

bool MotorController::readSDO(uint16_t index, uint8_t subIndex, uint32_t timeoutMs, std::vector<unsigned char>& receivedData) {
    unsigned char sdoResponseCs = 0;
    if (!m_canHandler.sendSdoReadRequest(m_nodeId, index, subIndex)) {
        m_lastError = "Failed to send SDO Read Request: " + m_canHandler.getLastError();
        logMessage("Error: " + m_lastError);
        return false;
    }
    if (!m_canHandler.receiveSdoResponse(m_nodeId, index, subIndex, timeoutMs, receivedData, sdoResponseCs)) {
        m_lastError = "Failed to receive SDO Read Response: " + m_canHandler.getLastError();
        logMessage("Error: " + m_lastError);
        return false;
    }
    if (sdoResponseCs == 0x80) {
        m_lastError = "SDO Read Aborted for " + std::to_string(index) + ":" + std::to_string(subIndex) + ". " + m_canHandler.getLastError();
        logMessage("Error: " + m_lastError);
        return false;
    }
    if ((sdoResponseCs >> 5) != 0x02) { // Check for Upload Response (010xxxxx)
        m_lastError = "Unexpected SDO response CS: 0x" + std::to_string(sdoResponseCs) + " for read from " + std::to_string(index) + ":" + std::to_string(subIndex);
        logMessage("Error: " + m_lastError);
        return false;
    }
    // m_lastError = "SDO Read successful for " + std::to_string(index) + ":" + std::to_string(subIndex); // Success logged by caller
    return true;
}

bool MotorController::setOperationMode(uint8_t mode) {
    std::vector<unsigned char> data = toBytes(static_cast<int8_t>(mode)); // Object 0x6060 is int8 in CiA402
    logMessage("Setting Operation Mode (0x6060:00) to " + std::to_string(mode));
    if (!writeSDO(MotorSdoObjects::MODES_OF_OPERATION_INDEX, MotorSdoObjects::MODES_OF_OPERATION_SUBINDEX, data)) {
        m_lastError = "Failed to set Operation Mode to " + std::to_string(mode) + ". " + m_lastError;
        // logMessage already called by writeSDO on failure
        return false;
    }
    m_lastError = "Operation Mode successfully set to " + std::to_string(mode) + ".";
    logMessage(m_lastError);
    return true;
}

bool MotorController::readStatusword(uint16_t& statusword) {
    std::vector<unsigned char> receivedData;
    if (!readSDO(MotorSdoObjects::STATUSWORD_INDEX, MotorSdoObjects::STATUSWORD_SUBINDEX, 100, receivedData)) {
        m_lastError = "Failed to read Statusword (0x6041:00). " + m_lastError;
        statusword = 0;
        return false;
    }
    if (receivedData.size() < sizeof(uint16_t)) {
        m_lastError = "Statusword SDO read returned insufficient data. Expected " + std::to_string(sizeof(uint16_t)) + " bytes, got " + std::to_string(receivedData.size());
        logMessage("Error: " + m_lastError);
        statusword = 0;
        return false;
    }
    statusword = fromBytes<uint16_t>(receivedData);
    m_lastError = "Successfully read Statusword."; // No need to log this one every time by default
    return true;
}

bool MotorController::enableMotor() {
    uint16_t status = 0;
    logMessage("Starting enable motor sequence...");
    if (!readStatusword(status)) {
        m_lastError = "EnableMotor: Failed to read statusword before sequence. " + m_lastError;
        logMessage("Error: " + m_lastError);
        return false;
    }
    logMessage("Initial Statusword: 0x" + std::to_string(status)); // Will be hex due to iomanip in poll

    logMessage("Setting Operation Mode to Profile Velocity (" + std::to_string(MotorSdoObjects::MODE_PROFILE_VELOCITY) + ") as default for enable sequence.");
    if (!setOperationMode(MotorSdoObjects::MODE_PROFILE_VELOCITY)) {
        m_lastError = "EnableMotor: Failed to set Profile Velocity mode. " + m_lastError;
        // logMessage already called by setOperationMode
        return false;
    }

    // CiA 402 State Machine Transitions:
    // Shutdown (0x06) -> Switch On (0x07) -> Operation Enable (0x0F)
    logMessage("Sending Controlword SHUTDOWN (0x0006) to 0x6040:00");
    if (!writeSDO(MotorSdoObjects::CONTROLWORD_INDEX, MotorSdoObjects::CONTROLWORD_SUBINDEX, toBytes<uint16_t>(0x0006))) {
        m_lastError = "EnableMotor: Failed to send SHUTDOWN (0x06) to Controlword. " + m_lastError;
        return false;
    }
    // TODO: Add delay and check Statusword for "Ready to Switch On"

    logMessage("Sending Controlword SWITCH ON (0x0007) to 0x6040:00");
    if (!writeSDO(MotorSdoObjects::CONTROLWORD_INDEX, MotorSdoObjects::CONTROLWORD_SUBINDEX, toBytes<uint16_t>(0x0007))) {
        m_lastError = "EnableMotor: Failed to send SWITCH ON (0x07) to Controlword. " + m_lastError;
        return false;
    }
    // TODO: Add delay and check Statusword for "Switched On"

    logMessage("Sending Controlword ENABLE OPERATION (0x000F) to 0x6040:00");
    if (!writeSDO(MotorSdoObjects::CONTROLWORD_INDEX, MotorSdoObjects::CONTROLWORD_SUBINDEX, toBytes<uint16_t>(0x000F))) {
        m_lastError = "EnableMotor: Failed to send ENABLE OPERATION (0x0F) to Controlword. " + m_lastError;
        return false;
    }
    // TODO: Add delay and check Statusword for "Operation Enabled"

    m_lastError = "Motor enable sequence successfully sent.";
    logMessage(m_lastError);
    return true;
}

bool MotorController::disableMotor() {
    logMessage("Sending Controlword DISABLE VOLTAGE (0x0000) to 0x6040:00");
    if (!writeSDO(MotorSdoObjects::CONTROLWORD_INDEX, MotorSdoObjects::CONTROLWORD_SUBINDEX, toBytes<uint16_t>(0x0000))) {
        m_lastError = "DisableMotor: Failed to send DISABLE VOLTAGE (0x00) to Controlword. " + m_lastError;
        return false;
    }
    m_lastError = "Motor disable sequence (Disable Voltage) successfully sent.";
    logMessage(m_lastError);
    return true;
}

bool MotorController::setTargetPosition(int32_t position) {
    logMessage("Setting Target Position (0x607A:00) to " + std::to_string(position));
    std::vector<unsigned char> data = toBytes(position);
    if (!writeSDO(MotorSdoObjects::TARGET_POSITION_INDEX, MotorSdoObjects::TARGET_POSITION_SUBINDEX, data)) {
        m_lastError = "Failed to set Target Position to " + std::to_string(position) + ". " + m_lastError;
        return false;
    }
    m_lastError = "Target Position successfully set to " + std::to_string(position) + ".";
    logMessage(m_lastError);
    // Note: Triggering movement might require additional Controlword changes (e.g., new_set_point bit)
    return true;
}

bool MotorController::readActualPosition(int32_t& position) {
    std::vector<unsigned char> receivedData;
    if (!readSDO(MotorSdoObjects::ACTUAL_POSITION_INDEX, MotorSdoObjects::ACTUAL_POSITION_SUBINDEX, 100, receivedData)) {
        m_lastError = "Failed to read Actual Position (0x6064:00). " + m_lastError;
        position = 0;
        return false;
    }
    if (receivedData.size() < sizeof(int32_t)) {
        m_lastError = "Actual Position SDO read returned insufficient data. Expected " + std::to_string(sizeof(int32_t)) + " bytes, got " + std::to_string(receivedData.size());
        logMessage("Error: " + m_lastError);
        position = 0;
        return false;
    }
    position = fromBytes<int32_t>(receivedData);
    m_lastError = "Successfully read Actual Position.";
    return true;
}

bool MotorController::setTargetVelocity(int32_t velocity) {
    logMessage("Setting Target Velocity (0x60FF:00) to " + std::to_string(velocity));
    std::vector<unsigned char> data = toBytes(velocity);
    if (!writeSDO(MotorSdoObjects::TARGET_VELOCITY_INDEX, MotorSdoObjects::TARGET_VELOCITY_SUBINDEX, data)) {
        m_lastError = "Failed to set Target Velocity to " + std::to_string(velocity) + ". " + m_lastError;
        return false;
    }
    m_lastError = "Target Velocity successfully set to " + std::to_string(velocity) + ".";
    logMessage(m_lastError);
    return true;
}

bool MotorController::setProfileAcceleration(uint32_t acceleration) {
    logMessage("Setting Profile Acceleration (0x6083:00) to " + std::to_string(acceleration));
    std::vector<unsigned char> data = toBytes(acceleration);
    if (!writeSDO(MotorSdoObjects::PROFILE_ACCELERATION_INDEX, MotorSdoObjects::PROFILE_ACCELERATION_SUBINDEX, data)) {
        m_lastError = "Failed to set Profile Acceleration to " + std::to_string(acceleration) + ". " + m_lastError;
        return false;
    }
    m_lastError = "Profile Acceleration successfully set to " + std::to_string(acceleration) + ".";
    logMessage(m_lastError);
    return true;
}

bool MotorController::setProfileDeceleration(uint32_t deceleration) {
    logMessage("Setting Profile Deceleration (0x6084:00) to " + std::to_string(deceleration));
    std::vector<unsigned char> data = toBytes(deceleration);
    if (!writeSDO(MotorSdoObjects::PROFILE_DECELERATION_INDEX, MotorSdoObjects::PROFILE_DECELERATION_SUBINDEX, data)) {
        m_lastError = "Failed to set Profile Deceleration to " + std::to_string(deceleration) + ". " + m_lastError;
        return false;
    }
    m_lastError = "Profile Deceleration successfully set to " + std::to_string(deceleration) + ".";
    logMessage(m_lastError);
    return true;
}

void MotorController::pollMotorStatus() {
    int32_t actualPos = 0;
    uint16_t status = 0;

    if (readActualPosition(actualPos)) {
        logMessage("Polled Actual Position: " + std::to_string(actualPos));
    } else {
        logMessage("Poll: Failed to read actual position. Error: " + getLastError());
    }

    if (readStatusword(status)) {
        std::stringstream ss;
        ss << "0x" << std::hex << std::setw(4) << std::setfill('0') << status;
        logMessage("Polled Statusword: " + ss.str());
    } else {
        logMessage("Poll: Failed to read statusword. Error: " + getLastError());
    }
}

std::string MotorController::getLastError() const {
    return m_lastError;
}
