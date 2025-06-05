#ifndef CAN_HANDLER_H
#define CAN_HANDLER_H

#include "libs/pcanbasic/PCANBasic.h" // Assuming this path is correct from INCLUDEPATH
#include <string>
#include <vector> // For potential data handling

// Define a structure for CAN messages for clarity, though PCANBasic might have its own
// For now, we'll assume we need to construct something compatible with CAN_Write
// A typical CAN message structure:
struct CANMessage {
    unsigned int id;    // CAN ID
    unsigned char len;  // Data Length Code (DLC)
    unsigned char data[8]; // Data bytes
    // PCANBasic might also require specifying message type (standard, extended, RTR, etc.)
    // For SDO, it's typically standard data frames.
};

class CANHandler
{
public:
    CANHandler();
    ~CANHandler();

    // Initialize the CAN interface
    // channel: The PCAN channel handle (e.g., PCAN_USBBUS1)
    // baudrate: The CAN baud rate (e.g., PCAN_BAUD_500K)
    bool connectDevice(void* pcanChannel, unsigned short baudrate);

    // Disconnect from the CAN interface
    void disconnectDevice();

    // Send an SDO Write message (expedited transfer)
    // nodeId: The ID of the target CANopen node
    // index: SDO Index
    // subIndex: SDO SubIndex
    // data: Data to write (up to 4 bytes for expedited transfer)
    bool sendSdoWriteRequest(unsigned short nodeId, unsigned short index, unsigned char subIndex, const std::vector<unsigned char>& data);

    // Send an SDO Read Request (Upload Initiate)
    // nodeId: The ID of the target CANopen node
    // index: SDO Index
    // subIndex: SDO SubIndex
    bool sendSdoReadRequest(unsigned short nodeId, unsigned short index, unsigned char subIndex);

    // Receive an SDO message (general purpose, handles write confirmations and read responses)
    // nodeId: The ID of the target CANopen node (to filter response)
    // expectedSdoIndex: SDO Index expected in the response
    // expectedSdoSubIndex: SDO SubIndex expected in the response
    // timeoutMs: Timeout in milliseconds to wait for the message
    // receivedData: Output vector for received data (for SDO reads)
    // sdoCommandSpecifierReceived: Output to store the SDO command specifier from the server (e.g., 0x60, 0x4F, 0x80)
    bool receiveSdoResponse(unsigned short nodeId, unsigned short expectedSdoIndex, unsigned char expectedSdoSubIndex, unsigned int timeoutMs, std::vector<unsigned char>& receivedData, unsigned char& sdoCommandSpecifierReceived);

    // Get the last error message
    std::string getLastError() const;

private:
    void* m_pcanChannel; // Store the PCAN channel handle being used
    bool m_isConnected;
    std::string m_lastError;

    // Helper to set error messages
    void setPcanError(TPCANStatus status, const std::string& context);

    // Placeholder for actual PCAN message structures if PCANBasic.h doesn't define them
    // For example, TPCANMsg might be used by CAN_Write / CAN_Read
};

#endif // CAN_HANDLER_H
