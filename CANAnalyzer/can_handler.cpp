// Prepending all includes and helpers needed for the full file overwrite
#include "can_handler.h"
#include <iostream>
#include <cstring>
#include <thread>
#include <chrono>
#include <sstream>
#include <iomanip>
#include <vector>
#include <map> // For m_sdoSimulatedObjects

// Temporary local helper (assuming it's still here or moved to a common place)
template<typename T>
static std::vector<unsigned char> sdoDataToBytes(T value) {
    std::vector<unsigned char> bytes(sizeof(T));
    const unsigned char* p = reinterpret_cast<const unsigned char*>(&value);
    for (size_t i = 0; i < sizeof(T); ++i) {
        bytes[i] = p[i];
    }
    return bytes;
}

const unsigned int SDO_REQUEST_COB_ID_BASE = 0x600;
const unsigned int SDO_RESPONSE_COB_ID_BASE = 0x580;

CANHandler::CANHandler() : m_pcanChannel(nullptr), m_isConnected(false),
                           m_lastSdoRequestType(LastSdoRequestType::NONE),
                           m_lastSdoIndex(0), m_lastSdoSubIndex(0)
{
    // Pre-populate some simulated SDO objects
    uint32_t key;
    // Device Name (0x1008:00)
    std::string devName = "DummyMaxon";
    std::vector<unsigned char> devNameBytes(devName.begin(), devName.end());
    key = (0x1008 << 16) | 0x00; m_sdoSimulatedObjects[key] = devNameBytes;
    // Modes of Operation (0x6060:00) - Set by motor controller, read by Modes of Op Display
    key = (0x6060 << 16) | 0x00; m_sdoSimulatedObjects[key] = sdoDataToBytes<int8_t>(0); // Default 0
    // Modes of Operation Display (0x6061:00)
    key = (0x6061 << 16) | 0x00; m_sdoSimulatedObjects[key] = sdoDataToBytes<int8_t>(0); // Default 0, should mirror 0x6060
    // Statusword (0x6041:00)
    key = (0x6041 << 16) | 0x00; m_sdoSimulatedObjects[key] = sdoDataToBytes<uint16_t>(0x0240); // Switch on disabled
    // Target Velocity (0x60FF:00)
    key = (0x60FF << 16) | 0x00; m_sdoSimulatedObjects[key] = sdoDataToBytes<int32_t>(0);
    // Profile Accel (0x6083:00)
    key = (0x6083 << 16) | 0x00; m_sdoSimulatedObjects[key] = sdoDataToBytes<uint32_t>(1000);
    // Profile Decel (0x6084:00)
    key = (0x6084 << 16) | 0x00; m_sdoSimulatedObjects[key] = sdoDataToBytes<uint32_t>(1000);
    // Actual Position (0x6064:00)
    key = (0x6064 << 16) | 0x00; m_sdoSimulatedObjects[key] = sdoDataToBytes<int32_t>(0);

}

CANHandler::~CANHandler() { /* ... */ }

void CANHandler::setPcanError(TPCANStatus status, const std::string& context) {
    if (status == PCAN_ERROR_OK) m_lastError = "PCAN_ERROR_OK";
    else if (status == PCAN_ERROR_XMTFULL) m_lastError = "PCAN_ERROR_XMTFULL: Transmit queue is full.";
    else { std::stringstream ss; ss << "0x" << std::hex << status; m_lastError = "Unknown PCAN error: " + ss.str(); }
    m_lastError = context + ": " + m_lastError;
    std::cerr << "Error: " << m_lastError << std::endl;
}

bool CANHandler::connectDevice(void* pcanChannel, unsigned short baudrate) {
    // ... (implementation as before) ...
    if (m_isConnected) { m_lastError = "Device already connected."; return false; }
    m_pcanChannel = pcanChannel;
    TPCANStatus status = CAN_Initialize(m_pcanChannel, baudrate, 0);
    if (status != PCAN_ERROR_OK) { setPcanError(status, "CAN_Initialize"); m_pcanChannel = nullptr; return false; }
    m_isConnected = true; m_lastError = "Successfully connected."; std::cout << m_lastError << std::endl; return true;
}

void CANHandler::disconnectDevice() {
    // ... (implementation as before) ...
    if (!m_isConnected) { m_lastError = "Device not connected."; return; }
    std::cout << "Simulating CAN_Uninitialize." << std::endl; m_lastError = "Disconnected (simulated).";
    m_isConnected = false; m_pcanChannel = nullptr;
}

// Base SDO Write Request
bool CANHandler::sendSdoWriteRequest(unsigned short nodeId, unsigned short index, unsigned char subIndex, const std::vector<unsigned char>& data) {
    if (!m_isConnected) { /* ... error ... */ return false; }
    if (data.size() > 4 && data.size() != 8) { /* allow 8 for some cases, or stick to 4 for expedited only */
        m_lastError = "SDO Write: Data size not valid for expedited transfer (must be <=4 bytes)."; return false;
    }
    m_lastSdoRequestType = LastSdoRequestType::WRITE;
    m_lastSdoIndex = index; m_lastSdoSubIndex = subIndex; m_lastSdoWriteData = data;

    unsigned char msgData[8]; memset(msgData, 0, sizeof(msgData));
    msgData[0] = (1 << 5) | ((4 - data.size()) << 2) | (1 << 1) | 1; // expedited, size indicated
    if (data.size() == 0) msgData[0] = (1 << 5) | (0 << 2) | (0 << 1) | 1; // expedited, size NOT indicated (for 0 byte write)

    msgData[1] = index & 0xFF; msgData[2] = (index >> 8) & 0xFF; msgData[3] = subIndex;
    if (!data.empty()) memcpy(&msgData[4], data.data(), data.size());

    size_t dlc = (data.size() <= 4) ? (4 + data.size()) : 8; // Simplified DLC for expedited

    // Logging
    std::stringstream ssData;
    for (size_t i = 0; i < dlc; ++i) ssData << "0x" << std::hex << std::setw(2) << std::setfill('0') << (int)msgData[i] << (i == (dlc - 1) ? "" : " ");
    std::cout << "[CAN TX SDO_WRITE] Node=" << nodeId << ", Idx=0x" << std::hex << index << ", Sub=0x" << (int)subIndex
              << ", COB=0x" << (SDO_REQUEST_COB_ID_BASE + nodeId) << ", DLC=" << std::dec << dlc
              << ", Data=[" << ssData.str() << "]" << std::dec << std::endl;

    // Simulate SDO write to internal storage
    uint32_t sdoKey = (static_cast<uint32_t>(index) << 16) | subIndex;
    m_sdoSimulatedObjects[sdoKey] = data;
    // If 0x6060 (Mode of Op) is written, update 0x6061 (Mode of Op Display)
    if (index == 0x6060 && subIndex == 0x00) {
        m_sdoSimulatedObjects[(0x6061 << 16) | 0x00] = data;
    }

    return true; // Assume PCAN_Write is OK
}
// Overloads for sendSdoWriteRequest
bool CANHandler::sendSdoWriteRequest(unsigned short nodeId, unsigned short index, unsigned char subIndex, uint8_t value) { return sendSdoWriteRequest(nodeId, index, subIndex, sdoDataToBytes(value)); }
bool CANHandler::sendSdoWriteRequest(unsigned short nodeId, unsigned short index, unsigned char subIndex, int8_t value) { return sendSdoWriteRequest(nodeId, index, subIndex, sdoDataToBytes(value)); }
bool CANHandler::sendSdoWriteRequest(unsigned short nodeId, unsigned short index, unsigned char subIndex, uint16_t value) { return sendSdoWriteRequest(nodeId, index, subIndex, sdoDataToBytes(value)); }
bool CANHandler::sendSdoWriteRequest(unsigned short nodeId, unsigned short index, unsigned char subIndex, int16_t value) { return sendSdoWriteRequest(nodeId, index, subIndex, sdoDataToBytes(value)); }
bool CANHandler::sendSdoWriteRequest(unsigned short nodeId, unsigned short index, unsigned char subIndex, uint32_t value) { return sendSdoWriteRequest(nodeId, index, subIndex, sdoDataToBytes(value)); }
bool CANHandler::sendSdoWriteRequest(unsigned short nodeId, unsigned short index, unsigned char subIndex, int32_t value) { return sendSdoWriteRequest(nodeId, index, subIndex, sdoDataToBytes(value)); }

// SDO Read Request
bool CANHandler::sendSdoReadRequest(unsigned short nodeId, unsigned short index, unsigned char subIndex) {
    if (!m_isConnected) { /* ... error ... */ return false; }
    m_lastSdoRequestType = LastSdoRequestType::READ;
    m_lastSdoIndex = index; m_lastSdoSubIndex = subIndex;

    unsigned char msgData[8]; memset(msgData, 0, sizeof(msgData));
    msgData[0] = (2 << 5); // 0x40 (initiate upload)
    msgData[1] = index & 0xFF; msgData[2] = (index >> 8) & 0xFF; msgData[3] = subIndex;

    // Logging
    std::stringstream ssDataRead;
    for (size_t i = 0; i < 8; ++i) ssDataRead << "0x" << std::hex << std::setw(2) << std::setfill('0') << (int)msgData[i] << (i == 7 ? "" : " ");
    std::cout << "[CAN TX SDO_READ] Node=" << nodeId << ", Idx=0x" << std::hex << index << ", Sub=0x" << (int)subIndex
              << ", COB=0x" << (SDO_REQUEST_COB_ID_BASE + nodeId) << ", DLC=8"
              << ", Data=[" << ssDataRead.str() << "]" << std::dec << std::endl;
    return true; // Assume PCAN_Write is OK
}

// Receive SDO Response - Enhanced Simulation
bool CANHandler::receiveSdoResponse(unsigned short nodeId, unsigned short expectedSdoIndex, unsigned char expectedSdoSubIndex, unsigned int timeoutMs, std::vector<unsigned char>& receivedData, unsigned char& sdoCommandSpecifierReceived) {
    if (!m_isConnected) { /* ... error ... */ return false; }
    receivedData.clear();
    unsigned int responseCobId = SDO_RESPONSE_COB_ID_BASE + nodeId;
    std::cout << "[CAN RX SDO_RESP] Simulating for Node=" << nodeId << ", ExpIdx=0x" << std::hex << expectedSdoIndex
              << ", ExpSub=0x" << (int)expectedSdoSubIndex << std::dec << " (Timeout: " << timeoutMs << "ms)..." << std::endl;
    std::this_thread::sleep_for(std::chrono::milliseconds(10)); // Simulate delay

    unsigned char simulatedCanMsgDataBytes[8]; memset(simulatedCanMsgDataBytes, 0, sizeof(simulatedCanMsgDataBytes));

    // Simulate response based on m_lastSdoRequestType and stored SDOs
    if (m_lastSdoRequestType == LastSdoRequestType::WRITE) {
        if (m_lastSdoIndex == expectedSdoIndex && m_lastSdoSubIndex == expectedSdoSubIndex) {
            sdoCommandSpecifierReceived = 0x60; // Write success
            simulatedCanMsgDataBytes[0] = sdoCommandSpecifierReceived;
            simulatedCanMsgDataBytes[1] = m_lastSdoIndex & 0xFF;
            simulatedCanMsgDataBytes[2] = (m_lastSdoIndex >> 8) & 0xFF;
            simulatedCanMsgDataBytes[3] = m_lastSdoSubIndex;
            // Data part (bytes 4-7) is zero for write confirmation.
        } else { /* Unexpected confirmation */ sdoCommandSpecifierReceived = 0x80; /* Abort */ }
    } else if (m_lastSdoRequestType == LastSdoRequestType::READ) {
        if (m_lastSdoIndex == expectedSdoIndex && m_lastSdoSubIndex == expectedSdoSubIndex) {
            uint32_t sdoKey = (static_cast<uint32_t>(m_lastSdoIndex) << 16) | m_lastSdoSubIndex;
            if (m_sdoSimulatedObjects.count(sdoKey)) {
                const auto& simData = m_sdoSimulatedObjects[sdoKey];
                if (simData.size() <= 4) { // Expedited response
                    int n = 4 - simData.size(); // Number of bytes not used
                    sdoCommandSpecifierReceived = (2 << 5) | (n << 2) | (1 << 1) | 1; // scs=2, e=1, s=1
                    simulatedCanMsgDataBytes[0] = sdoCommandSpecifierReceived;
                    simulatedCanMsgDataBytes[1] = m_lastSdoIndex & 0xFF;
                    simulatedCanMsgDataBytes[2] = (m_lastSdoIndex >> 8) & 0xFF;
                    simulatedCanMsgDataBytes[3] = m_lastSdoSubIndex;
                    memcpy(&simulatedCanMsgDataBytes[4], simData.data(), simData.size());
                } else { /* Segmented not supported by this sim */ sdoCommandSpecifierReceived = 0x80; /* Abort */ }
            } else { /* Object not in sim store */ sdoCommandSpecifierReceived = 0x80; /* Abort - Object does not exist */
                // Populate abort code for object not existing: 0x06020000
                 simulatedCanMsgDataBytes[4] = 0x00; simulatedCanMsgDataBytes[5] = 0x00;
                 simulatedCanMsgDataBytes[6] = 0x02; simulatedCanMsgDataBytes[7] = 0x06;
            }
        } else { /* Unexpected confirmation */ sdoCommandSpecifierReceived = 0x80; /* Abort */ }
    } else { /* No prior request or unknown */ sdoCommandSpecifierReceived = 0x80; /* Abort */ }

    // Reset last request type after processing
    m_lastSdoRequestType = LastSdoRequestType::NONE;

    // --- Start of SDO parsing logic (from before, now using sdoCommandSpecifierReceived directly) ---
    unsigned char* dataBytes = simulatedCanMsgDataBytes;
    // sdoCommandSpecifierReceived is already set by simulation logic above.

    std::stringstream ssRxData;
    for (size_t i = 0; i < 8; ++i) ssRxData << "0x" << std::hex << std::setw(2) << std::setfill('0') << (int)dataBytes[i] << (i == 7 ? "" : " ");
    std::cout << "[CAN RX SDO_RESP Simulated] COB=0x" << std::hex << responseCobId << ", Data=[" << ssRxData.str() << "]" << std::dec << std::endl;


    unsigned short responseIndex = *((unsigned short*)&dataBytes[1]);
    unsigned char responseSubIndex = dataBytes[3];

    if (responseIndex == expectedSdoIndex && responseSubIndex == expectedSdoSubIndex) {
        unsigned char scs_bits = (sdoCommandSpecifierReceived >> 5) & 0x07;
        if (sdoCommandSpecifierReceived == 0x80) { /* ... SDO Abort handling ... */
            uint32_t abortCode = ((uint32_t)dataBytes[7] << 24) | ((uint32_t)dataBytes[6] << 16) | ((uint32_t)dataBytes[5] << 8) | ((uint32_t)dataBytes[4]);
            std::stringstream ss_abort; ss_abort << "0x" << std::hex << abortCode;
            m_lastError = "SDO Aborted. Idx:0x" + std::to_string(responseIndex) + " Sub:0x" + std::to_string(responseSubIndex) + " Code:" + ss_abort.str();
            std::cerr << "Error: " << m_lastError << std::endl; return false;
        } else if (scs_bits == 3) { /* ... Write Confirmation ... */
            m_lastError = "SDO Write Confirmed."; std::cout << m_lastError << std::endl; receivedData.clear(); return true;
        } else if (scs_bits == 2) { /* ... Read Confirmation ... */
            bool expedited = (sdoCommandSpecifierReceived >> 0) & 1; bool size_indicated = (sdoCommandSpecifierReceived >> 1) & 1;
            if (expedited && size_indicated) {
                int num_empty_bytes = (sdoCommandSpecifierReceived >> 2) & 0x03; int data_len = 4 - num_empty_bytes;
                receivedData.assign(&dataBytes[4], &dataBytes[4] + data_len);
                m_lastError = "SDO Read successful."; std::cout << m_lastError << " Bytes:" << data_len << std::endl; return true;
            } else { /* ... error non-expedited/size ... */
                m_lastError = "SDO Read: Non-exp/size-not-indicated."; std::cerr << "Error: " << m_lastError << std::endl; return false;
            }
        } else { /* ... error unknown CS ... */
            std::stringstream ss_cs; ss_cs << "0x" << std::hex << (int)sdoCommandSpecifierReceived;
            m_lastError = "SDO: Unknown/unsupported CS " + ss_cs.str(); std::cerr << "Error: " << m_lastError << std::endl; return false;
        }
    } else { /* ... error Index/SubIndex mismatch ... */
        m_lastError = "SDO Resp Idx/Sub mismatch. Exp " + std::to_string(expectedSdoIndex) + ":" + std::to_string(expectedSdoSubIndex) +
                      ", Got " + std::to_string(responseIndex) + ":" + std::to_string(responseSubIndex);
        std::cerr << "Error: " << m_lastError << std::endl; return false;
    }
    // Fallback, should be handled by logic above
    return false;
}

std::string CANHandler::getLastError() const { return m_lastError; }
