#include "can_handler.h"
#include <iostream> // For basic error printing, replace with proper logging later
#include <cstring>  // For memcpy
#include <thread>   // For std::this_thread::sleep_for
#include <chrono>   // For std::chrono::milliseconds
#include <sstream>  // For std::stringstream (hex conversion)
#include <iomanip>  // For std::hex, std::setfill, std::setw (though sstream handles hex directly)

// PCAN Channel - this would be a specific channel like PCAN_USBBUS1, etc.
// For the dummy, we just pass it around as void*.
// In a real application, this might be configurable.

// SDO COB-IDs (CANopen defaults)
// Request from client to server (nodeId is server's ID)
const unsigned int SDO_REQUEST_COB_ID_BASE = 0x600;
// Response from server to client
const unsigned int SDO_RESPONSE_COB_ID_BASE = 0x580;


CANHandler::CANHandler() : m_pcanChannel(nullptr), m_isConnected(false) {}

CANHandler::~CANHandler()
{
    if (m_isConnected) {
        disconnectDevice();
    }
}

void CANHandler::setPcanError(TPCANStatus status, const std::string& context)
{
    // char errorText[256]; // Was unused as CAN_GetErrorText is not part of our dummy PCANBasic.h API yet
    // In a real PCANBasic library, CAN_GetErrorText(status, 0, errorText); would be used.
    // For our dummy, we'll simulate this.
    if (status == PCAN_ERROR_OK) {
        m_lastError = "PCAN_ERROR_OK";
    } else if (status == PCAN_ERROR_XMTFULL) {
        m_lastError = "PCAN_ERROR_XMTFULL: Transmit queue is full.";
    } else {
        std::stringstream ss_err_code;
        ss_err_code << "0x" << std::hex << status;
        m_lastError = "Unknown PCAN error: " + ss_err_code.str();
    }
    m_lastError = context + ": " + m_lastError;
    std::cerr << "Error: " << m_lastError << std::endl;
}

bool CANHandler::connectDevice(void* pcanChannel, unsigned short baudrate)
{
    if (m_isConnected) {
        m_lastError = "Device already connected.";
        return false;
    }

    m_pcanChannel = pcanChannel; // Store the channel handle

    // Call the dummy CAN_Initialize
    // The third parameter (...) in the dummy is variadic, so we can pass 0 or nullptr.
    TPCANStatus status = CAN_Initialize(m_pcanChannel, baudrate, 0);

    if (status != PCAN_ERROR_OK) {
        setPcanError(status, "CAN_Initialize");
        m_pcanChannel = nullptr;
        return false;
    }

    m_isConnected = true;
    m_lastError = "Successfully connected to PCAN channel.";
    std::cout << m_lastError << std::endl;
    return true;
}

void CANHandler::disconnectDevice()
{
    if (!m_isConnected) {
        m_lastError = "Device not connected.";
        return;
    }

    std::cout << "Simulating CAN_Uninitialize for channel." << std::endl;
    m_lastError = "Successfully disconnected (simulated).";

    m_isConnected = false;
    m_pcanChannel = nullptr;
}

bool CANHandler::sendSdoWriteRequest(unsigned short nodeId, unsigned short index, unsigned char subIndex, const std::vector<unsigned char>& data)
{
    if (!m_isConnected) {
        m_lastError = "Cannot send SDO Write: Not connected.";
        return false;
    }
    if (data.size() > 4) {
        m_lastError = "Cannot send SDO Write: Data size exceeds 4 bytes for expedited transfer.";
        return false;
    }

    unsigned char msgData[8];
    memset(msgData, 0, sizeof(msgData));

    msgData[0] = (1 << 5) | ((4 - data.size()) << 2) | (1 << 1) | 1;
    msgData[1] = index & 0xFF;
    msgData[2] = (index >> 8) & 0xFF;
    msgData[3] = subIndex;

    if (!data.empty()) {
        memcpy(&msgData[4], data.data(), data.size());
    }

    unsigned int cobId = SDO_REQUEST_COB_ID_BASE + nodeId;
    std::cout << "Simulating CAN_Write (SDO Write Request): ID=0x" << std::hex << cobId
              << ", DLC=" << std::dec << (4 + data.size())
              << ", Data=[";
    for (size_t i = 0; i < (4 + data.size()); ++i) {
        std::cout << "0x" << std::hex << (int)msgData[i] << (i == (3 + data.size()) ? "" : " ");
    }
    std::cout << "]" << std::dec << std::endl;

    TPCANStatus status = PCAN_ERROR_OK;

    if (status != PCAN_ERROR_OK) {
        setPcanError(status, "CAN_Write (SDO Write Request)");
        return false;
    }
    return true;
}

bool CANHandler::sendSdoReadRequest(unsigned short nodeId, unsigned short index, unsigned char subIndex) {
    if (!m_isConnected) {
        m_lastError = "Cannot send SDO Read Request: Not connected.";
        return false;
    }

    unsigned char msgData[8];
    memset(msgData, 0, sizeof(msgData));
    msgData[0] = (2 << 5); // 0x40
    msgData[1] = index & 0xFF;
    msgData[2] = (index >> 8) & 0xFF;
    msgData[3] = subIndex;

    unsigned int dlc = 8;
    unsigned int cobId = SDO_REQUEST_COB_ID_BASE + nodeId;
    std::cout << "Simulating CAN_Write (SDO Read Request): ID=0x" << std::hex << cobId
              << ", DLC=" << std::dec << dlc
              << ", Data=[";
    for (size_t i = 0; i < dlc; ++i) {
        std::cout << "0x" << std::hex << (int)msgData[i] << (i == (dlc - 1) ? "" : " ");
    }
    std::cout << "]" << std::dec << std::endl;

    TPCANStatus status = PCAN_ERROR_OK;

    if (status != PCAN_ERROR_OK) {
        setPcanError(status, "CAN_Write (SDO Read Request)");
        return false;
    }
    return true;
}

bool CANHandler::receiveSdoResponse(unsigned short nodeId, unsigned short expectedSdoIndex, unsigned char expectedSdoSubIndex, unsigned int timeoutMs, std::vector<unsigned char>& receivedData, unsigned char& sdoCommandSpecifierReceived)
{
    if (!m_isConnected) {
        m_lastError = "Cannot receive SDO response: Not connected.";
        return false;
    }

    receivedData.clear();
    unsigned int expectedCobId = SDO_RESPONSE_COB_ID_BASE + nodeId;

    std::cout << "Simulating CAN_Read for SDO response (Timeout: " << timeoutMs << "ms)..." << std::endl;
    std::this_thread::sleep_for(std::chrono::milliseconds(10));

    TPCANStatus status = PCAN_ERROR_OK; // Simulate successful read from CAN bus

    if (status == PCAN_ERROR_OK) {
        // --- SIMULATION of the content of the received message ---
        unsigned char simulatedCanMsgDataBytes[8];
        memset(simulatedCanMsgDataBytes, 0, sizeof(simulatedCanMsgDataBytes));
        unsigned int actualReceivedCobId = expectedCobId; // Simulate receiving the correct COB-ID

        // Default simulation: SDO Write success (0x60) for the requested OD entry
        sdoCommandSpecifierReceived = 0x60;
        simulatedCanMsgDataBytes[0] = sdoCommandSpecifierReceived;
        simulatedCanMsgDataBytes[1] = expectedSdoIndex & 0xFF;
        simulatedCanMsgDataBytes[2] = (expectedSdoIndex >> 8) & 0xFF;
        simulatedCanMsgDataBytes[3] = expectedSdoSubIndex;

        // Example of how to make simulation smarter for a specific read request:
        // if (expectedSdoIndex == 0x1008 && expectedSdoSubIndex == 0x00) { // Manufacturer Device Name
        //    sdoCommandSpecifierReceived = 0x43; // Expedited, 4 bytes (n=0)
        //    simulatedCanMsgDataBytes[0] = sdoCommandSpecifierReceived;
        //    simulatedCanMsgDataBytes[4] = 'D'; simulatedCanMsgDataBytes[5] = 'E';
        //    simulatedCanMsgDataBytes[6] = 'M'; simulatedCanMsgDataBytes[7] = 'O';
        // }
        // --- END SIMULATION of message content ---

        if (actualReceivedCobId == expectedCobId) {
            unsigned char* dataBytes = simulatedCanMsgDataBytes; // In real code, this pointer comes from the actual TPCANMsg

            sdoCommandSpecifierReceived = dataBytes[0]; // Store the command specifier from the message

            unsigned short responseIndex = *((unsigned short*)&dataBytes[1]);
            unsigned char responseSubIndex = dataBytes[3];

            if (responseIndex == expectedSdoIndex && responseSubIndex == expectedSdoSubIndex) {
                unsigned char scs_bits = (sdoCommandSpecifierReceived >> 5) & 0x07; // Server Command Specifier bits

                if (sdoCommandSpecifierReceived == 0x80) { // SDO Abort
                    uint32_t abortCode = ((uint32_t)dataBytes[7] << 24) | ((uint32_t)dataBytes[6] << 16) | ((uint32_t)dataBytes[5] << 8) | ((uint32_t)dataBytes[4]);
                    std::stringstream ss_abort; ss_abort << "0x" << std::hex << abortCode;
                    m_lastError = "SDO Aborted. Index: 0x" + std::to_string(responseIndex) + " SubIndex: 0x" + std::to_string(responseSubIndex) + " Code: " + ss_abort.str();
                    std::cerr << "Error: " << m_lastError << std::endl;
                    return false;
                } else if (scs_bits == 3) { // Download Response (Write Confirmation)
                    m_lastError = "SDO Write Confirmed.";
                    std::cout << m_lastError << std::endl;
                    receivedData.clear(); // No data payload in a write confirmation
                    return true;
                } else if (scs_bits == 2) { // Upload Response (Read Confirmation)
                    bool expedited = (sdoCommandSpecifierReceived >> 0) & 1;
                    bool size_indicated = (sdoCommandSpecifierReceived >> 1) & 1;
                    if (expedited && size_indicated) {
                        int num_empty_bytes = (sdoCommandSpecifierReceived >> 2) & 0x03;
                        int data_len = 4 - num_empty_bytes;
                        receivedData.assign(&dataBytes[4], &dataBytes[4] + data_len);
                        m_lastError = "SDO Read successful.";
                        std::cout << m_lastError << " Data bytes: " << data_len << std::endl;
                        return true;
                    } else {
                        m_lastError = "SDO Read: Non-expedited or size not indicated - not supported by this simple parser.";
                        std::cerr << "Error: " << m_lastError << std::endl;
                        return false;
                    }
                } else {
                    std::stringstream ss_cs; ss_cs << "0x" << std::hex << (int)sdoCommandSpecifierReceived;
                    m_lastError = "Received SDO: Unknown/unsupported command specifier " + ss_cs.str();
                    std::cerr << "Error: " << m_lastError << std::endl;
                    return false;
                }
            } else { // Index/SubIndex mismatch
                m_lastError = "Received SDO message for unexpected Index/SubIndex. Expected " + std::to_string(expectedSdoIndex) + ":" + std::to_string(expectedSdoSubIndex) + ", Got " + std::to_string(responseIndex) + ":" + std::to_string(responseSubIndex);
                std::cerr << "Error: " << m_lastError << std::endl;
                return false;
            }
        } else { // COB-ID mismatch
            std::stringstream ss_exp_cob, ss_got_cob;
            ss_exp_cob << "0x" << std::hex << expectedCobId;
            ss_got_cob << "0x" << std::hex << actualReceivedCobId; // In real code, use actual received COB-ID
            m_lastError = "Received CAN message with unexpected COB-ID. Expected " + ss_exp_cob.str() + ", Got " + ss_got_cob.str();
            std::cerr << "Error: " << m_lastError << std::endl;
            return false;
        }
    } else if (status == PCAN_ERROR_XMTFULL) { // Or PCAN_ERROR_QRCVEMPTY for read timeout
        setPcanError(status, "CAN_Read (SDO Response) - No message received (simulating timeout or queue full)");
        return false;
    } else { // Other PCAN errors
        setPcanError(status, "CAN_Read (SDO Response)");
        return false;
    }
    return false;
}

std::string CANHandler::getLastError() const
{
    return m_lastError;
}

// Note:
// The SDO response simulation logic in receiveSdoResponse is still basic.
// For more robust testing without a real CAN device, this simulation would need to be
// more context-aware (i.e., respond differently based on the SDO request that was made).
// For example, if sendSdoReadRequest was just called for OD entry 0x1008,
// the simulation should craft a read response for 0x1008 with some data.
// Currently, it defaults to simulating a write confirmation.
