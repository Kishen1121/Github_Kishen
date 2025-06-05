#ifndef PCANBASIC_H
#define PCANBASIC_H

// Dummy enum for PCANBasic
typedef enum {
    PCAN_ERROR_OK = 0x00000,
    PCAN_ERROR_XMTFULL = 0x00001
} TPCANStatus;

// Dummy function prototype
TPCANStatus CAN_Initialize(void* Channel, unsigned short Baudrate, ...);

#endif // PCANBASIC_H
