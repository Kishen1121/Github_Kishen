#ifndef MOTOR_CONTROLLER_H
#define MOTOR_CONTROLLER_H

#include "can_handler.h" // To use SDO communication
#include <vector>
#include <string>
#include <cstdint> // For int32_t, uint16_t etc.

// Define some common CANopen Object Dictionary entries for a motor (examples)
// These would be specific to the motor drive being controlled.
namespace MotorSdoObjects {
    // Controlword (Index 0x6040, SubIndex 0x00)
    const uint16_t CONTROLWORD_INDEX = 0x6040;
    const uint8_t CONTROLWORD_SUBINDEX = 0x00;

    // Statusword (Index 0x6041, SubIndex 0x00) - For reading motor state
    const uint16_t STATUSWORD_INDEX = 0x6041;
    const uint8_t STATUSWORD_SUBINDEX = 0x00;

    // Target Position (Index 0x607A, SubIndex 0x00) - Profile Position Mode
    const uint16_t TARGET_POSITION_INDEX = 0x607A;
    const uint8_t TARGET_POSITION_SUBINDEX = 0x00;

    // Actual Position (Index 0x6064, SubIndex 0x00) - Position Actual Value
    const uint16_t ACTUAL_POSITION_INDEX = 0x6064;
    const uint8_t ACTUAL_POSITION_SUBINDEX = 0x00;

    // Modes of Operation (Index 0x6060, SubIndex 0x00)
    const uint16_t MODES_OF_OPERATION_INDEX = 0x6060;
    const uint8_t MODES_OF_OPERATION_SUBINDEX = 0x00;
    // Example modes: 1 = Profile Position Mode, 3 = Profile Velocity Mode, etc.
    const int8_t MODE_PROFILE_POSITION = 1;
}

class MotorController
{
public:
    // Node ID of the motor drive on the CANopen network
    MotorController(CANHandler& canHandler, uint16_t nodeId);
    ~MotorController();

    // --- High-Level Motor Control Functions ---

    // Enable the motor (sequence to reach "Operation Enabled" state)
    bool enableMotor();

    // Disable the motor (sequence to reach "Switch On Disabled" state)
    bool disableMotor();

    // Set the target position for the motor
    // position: The desired position value (drive-specific units)
    bool setTargetPosition(int32_t position);

    // Read the actual position of the motor
    // position: Output parameter to store the read position
    bool readActualPosition(int32_t& position);

    // Set the mode of operation (e.g., Profile Position Mode)
    bool setModeOfOperation(int8_t mode);

    // Read the statusword
    bool readStatusword(uint16_t& statusword);


    // --- Polling Logic ---
    // For simplicity, a manual poll function. A real app might use QTimer.
    void pollMotorStatus(); // Example: reads and prints actual position & status

    std::string getLastError() const;

private:
    CANHandler& m_canHandler;
    uint16_t m_nodeId; // CANopen Node ID of the motor drive
    std::string m_lastError;

    // Helper to send SDO write messages
    bool writeSDO(uint16_t index, uint8_t subIndex, const std::vector<unsigned char>& data);

    // Helper to send SDO read messages (conceptual, SDO receive in CANHandler is for write confirmation)
    // For reading data, we'd need a different SDO exchange pattern (Upload Initiate).
    // The current CANHandler::receiveSDOMessage is for confirming a *write*.
    // We'll need to adapt or extend CANHandler for SDO reads.
    // For now, this will be a placeholder.
    bool readSDO(uint16_t index, uint8_t subIndex, uint32_t timeoutMs, std::vector<unsigned char>& receivedData);

    // Helper to interpret SDO Abort codes if receiveSDOMessage is enhanced
    std::string sdoAbortCodeToString(uint32_t abortCode);
};

#endif // MOTOR_CONTROLLER_H
