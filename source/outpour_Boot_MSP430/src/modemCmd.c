/** 
 * @file modemCmd.c
 * \n Source File
 * \n Outpour MSP430 Bootloader Firmware
 * 
 * \brief Handle sending commands to and getting responses from 
 *        the modem.  Handle the details of modem message
 *        protocol and format details.  
 */

#include "outpour.h"

/***************************
 * Module Data Definitions
 **************************/

/**
 * \def ISR_BUF_SIZE 
 * \brief Define the size of the UART receive buffer. 
 */
#define ISR_BUF_SIZE ((uint8_t)OTA_PAYLOAD_BUF_LENGTH+16)

/**
 * \def MODEM_CMD_START_BYTE 
 * \brief Modem Command start byte 
 */
#define	MODEM_CMD_START_BYTE	((uint8_t)0x3c)

/**
 * \def MODEM_RESP_START_BYTE 
 * \brief Modem Response start byte 
 */
#define	MODEM_RESP_START_BYTE	((uint8_t)0x3e)

/**
 * \def MODEM_CMD_END_BYTE 
 * \brief Modem Command/Response end byte 
 */
#define	MODEM_CMD_END_BYTE		((uint8_t)0x3b)

/**
 * \def MODEM_TX_RX_TIMEOUT_IN_SEC
 * \brief define how long to wait before declaring an error for 
 *        a modem tx/rx session.
 */
#define MODEM_TX_RX_TIMEOUT_IN_SEC ((uint32_t)10*(SYS_TICKS_PER_SECOND))

/**
 * \def MODEM_CMD_MAX_RETRIES
 * \brief Define how many times to resend a command and wait for 
 *        response after an unsuccessful attempt.
 */
#define MODEM_CMD_MAX_RETRIES ((uint8_t)3)

/**
 * \typedef modemCmdData_t 
 * \brief Contains module data 
 */
typedef struct modemCmdData_s {

    bool busy;                         /**< signals we are busy sending a modem msg */
    modem_command_t modemCmdId;        /**< the cmd we are sending to the modem */
    sys_tick_t sendTimestamp;          /**< time we enabled the tx isr */

    uint8_t retryCount;                /**< how many tries to tx/rx the msg */
    bool msgTxRxFailed;                /**< the message failed to tx or rx properly */
    uint16_t crc;                      /**< crc16 calculated value on tx or rx msg */

    uint8_t *txHeaderP;                /**< buffer for holding modem msg cmd header */
    uint8_t txHeaderLength;            /**< length of msg tx cmd header */
    uint16_t txMsgPayloadLength;       /**< modem tx msg data payload length (if any) */
    uint8_t expectedResponseLength;    /**< expected msg response length */

    bool txIsrMsgComplete;             /**< A complete msg has been transmitted */
    uint8_t *txPayloadP;               /**< pointer to tx data payload */
    uint16_t txIsrDataIndex;           /**< Tx Isr Data Index into buffer */

    bool rxIsrMsgComplete;             /**< A complete msg has been received */
    uint8_t *rxBufP;                   /**< Buffer where the Rx ISR puts data */
    uint16_t rxIsrDataIndex;           /**< Rx ISR Data Index into buffer */
} modemCmdData_t;

/****************************
 * Module Data Declarations
 ***************************/

/**
 * \var isrCommBuf
 * \brief This buffer location is specified in the linker
 *        command file to live right below the stack space in
 *        RAM.
 */
#pragma SET_DATA_SECTION(".commbufs")
uint8_t isrCommBuf[ISR_BUF_SIZE];  /**< Common buffer used to tx and rx data to/from */
#pragma SET_DATA_SECTION()

/**
* \var mcData
* \brief Declare a data object to "house" data for this module.
*/
static modemCmdData_t mcData;

/*************************
 * Module Prototypes
 ************************/

static bool modemCmdProcessRxMsg(void);
static void modemCmdIsrRestart(void);
static void modemCmdCleanup(void);
static void USCI0RX_ISR(void);
static void USCI0TX_ISR(void);

/***************************
 * Module Public Functions
 **************************/

/**
* \brief Modem Cmd executive.  Called on the system tick to
*        manage a single modem transmit/receive transaction. If
*        no transaction is in progress, just returns. Detects
*        when a transaction is timeout or completed
*        successfully.
*/
void modemCmd_exec(void) {

    bool done = false;
    bool retryNeeded = false;
    bool msgOk = false;

    // If we are NOT currently busy with a modem transaction - just return.
    if (!mcData.busy) {
        return;
    }

    // For the bootloader we use polling and not interrupts for tx/rx
    // on the MSP430 UART.
    USCI0RX_ISR();
    USCI0TX_ISR();

    // Check if modem tx/rx transaction is complete
    if (mcData.txIsrMsgComplete && mcData.rxIsrMsgComplete) {
        msgOk = modemCmdProcessRxMsg();
        if (msgOk) {
            // Success! Message transaction complete
            done = true;
        } else {
            retryNeeded = true;
        }
    }
    // Check for a timeout
    else if (GET_ELAPSED_SYS_TICKS(mcData.sendTimestamp) > MODEM_TX_RX_TIMEOUT_IN_SEC) {
        retryNeeded = true;
    }

    if (retryNeeded) {
        // Check for a max retry error for current tx/rx transaction
        if (mcData.retryCount < MODEM_CMD_MAX_RETRIES) {
            mcData.retryCount++;
            // Resend the command
            modemCmdIsrRestart();
        } else {
            mcData.msgTxRxFailed = true;
            done = true;
        }
    }

    if (done) {
        modemCmdCleanup();
    }
}

/**
* \brief For the bootloader, it uses polling and not interrupts 
*        to rx and tx.  This function checks if a data byte has
*        been received or if one needs to be transmitted.
* \ingroup PUBLIC_API
*/
void modemCmd_pollUart(void) {
    // If we are NOT currently busy with a modem transaction - just return.
    if (!mcData.busy) {
        return;
    }
    USCI0RX_ISR();
    USCI0TX_ISR();
}

/**
* \brief One time initialization for module.  Call one time 
*        after system starts or (optionally) wakes.
* \ingroup PUBLIC_API
*/
void modemCmd_init(void) {
    memset(&mcData, 0, sizeof(modemCmdData_t));
    // The tx and rx use a common buffer
    mcData.txHeaderP = isrCommBuf;
    mcData.rxBufP = isrCommBuf;
}

/**
* \brief Start a new data tx/rx transaction with the modem.
* \ingroup PUBLIC_API
* 
* @param writeCmdP Pointer to a modemCmdWriteData_t object 
*                  initialized with the cmd data to transmit.
* 
* @return bool Returns true if the modem is not busy and a 
*         message transaction session was started.
*/

bool modemCmd_write(modemCmdWriteData_t *writeCmdP) {
    // Default the message data length to 1;
    uint8_t msgDataLength = 1;

    // If not busy, mark as busy.  Other return false if already busy.
    if (!mcData.busy) {
        mcData.busy = true;
    } else {
        return false;
    }
    // Pre-zero the packet so we don't have to worry about initializing 
    // any zero fields in the packet (in order to save code space).
    memset(&mcData.txHeaderP[0], 0, 32);
    // Save the command ID
    mcData.modemCmdId = writeCmdP->cmd;

    // Add the start byte and the command to the message.
    mcData.txHeaderP[0] = MODEM_CMD_START_BYTE;
    mcData.txHeaderP[1] = writeCmdP->cmd;

    // Prepare the command to send to the modem.
    switch (writeCmdP->cmd) {
    case M_COMMAND_PING:
        {
            mcData.crc = ((uint16_t)0x0);          // precalculated crc
            mcData.expectedResponseLength = 5;     // start,cmd,crc[2],end
        }
        break;
    case M_COMMAND_SEND_SOS:
        {
            // This is the standard definition of the Outpour message header.
            // This header starts at byte 5 of the message going to the modem.
            // We can't fill out the complete message when sending.
            // But we fill in the porductId and the version number of the bootloader
            // uint8_t StartByte;              /**< header byte 0  => byte 5 of message */
            // uint8_t msgId;                  /**< header byte 1  => byte 6 of message */
            // uint8_t productId;              /**< header byte 2  => byte 7 of message */
            // uint8_t GMTsecond;              /**< header byte 3  => byte 8 of message */
            // uint8_t GMTminute;              /**< header byte 4  => byte 9 of message */
            // uint8_t GMThour;                /**< header byte 5  => byte 10 of message */
            // uint8_t GMTday;                 /**< header byte 6  => byte 11 of message */
            // uint8_t GMTmonth;               /**< header byte 7  => byte 12 of message */
            // uint8_t GMTyear;                /**< header byte 8  => byte 13 of message */
            // uint8_t fwMajor;                /**< header byte 9  => byte 14 of message */
            // uint8_t fwMinor;                /**< header byte 10 => byte 15 of message */
            // uint8_t daysActivatedMsb;       /**< header byte 11 => byte 16 of message */
            // uint8_t daysActivatedLsb;       /**< header byte 12 => byte 17 of message */
            // uint8_t weeks;                  /**< header byte 13 => byte 18 of message */
            // uint8_t reserve2;               /**< header byte 14 => byte 19 of message */
            // uint8_t reserve3;               /**< header byte 15 => byte 20 of message */

            uint16_t size = 16;
            // mcData.txHeaderP[2] = 0;                  // Payload Size: bits 24-31 - already zero'd
            // mcData.txHeaderP[3] = 0;                  // Payload Size: bits 16-23 - already zero'd
            mcData.txHeaderP[4] = (size >> 8) & 0xFF;    // Payload Size: bits 8-15
            mcData.txHeaderP[5] = size & 0xFF;           // Payload Size: bits 0-7

            // First fill Outpour message header with known values.
            memset(&mcData.txHeaderP[6], 0x55, 16);
            // Now fill in individual values that we must have
            mcData.txHeaderP[6] = 0x01;                       // Payload start byte
            mcData.txHeaderP[7] = writeCmdP->payloadMsgId;    // Payload message type
            mcData.txHeaderP[8] = OUTPOUR_PRODUCT_ID;         // Outpour Product ID
            mcData.txHeaderP[15] = FW_VERSION_MAJOR;          // FW MAJOR
            mcData.txHeaderP[16] = FW_VERSION_MINOR;          // FW MINOR

            // Total length of the data portion sent to modem is 21 bytes.
            msgDataLength = 5 + 16;
            mcData.crc = gen_crc16((uint8_t *)&(mcData.txHeaderP[1]), msgDataLength);
            mcData.expectedResponseLength = 5;                // start,cmd,crc[2],end
        }
        break;
    case M_COMMAND_MODEM_STATUS:
        {
            mcData.crc = ((uint16_t)0xc181);    // precalculated crc
            mcData.expectedResponseLength = 15; // start,cmd,status[10],crc[2],end
        }
        break;
    case M_COMMAND_MESSAGE_STATUS:
        {
            mcData.crc = ((uint16_t)0x0140);    // pre-calculated crc
            mcData.expectedResponseLength = 23; // start,cmd,status[18],crc[2],end
        }
        break;
    case M_COMMAND_GET_INCOMING_PARTIAL:
        {
            // Setup receive expectations for modem
            mcData.txHeaderP[4] = (writeCmdP->payloadOffset >> 8) & 0xff;   // offset bits 8-15
            mcData.txHeaderP[5] = writeCmdP->payloadOffset & 0xff;          // offset bits 0-7
            mcData.txHeaderP[8] = (writeCmdP->payloadLength >> 8) & 0xff;   // size bits 8-15
            mcData.txHeaderP[9] = writeCmdP->payloadLength & 0xff;          // size bits 0-7
            // Total length of the data portion sent to modem is 9 bytes.
            msgDataLength = 9;
            mcData.crc = gen_crc16(&(mcData.txHeaderP[1]), msgDataLength);
            mcData.expectedResponseLength = 13 + writeCmdP->payloadLength;  // start,cmd,len[4],remaining[4],payload[sizeInBytes],crc[2],end
        }
        break;
    case M_COMMAND_DELETE_INCOMING:
        {
            mcData.crc = 0xf141;                // precalculated crc
            mcData.expectedResponseLength = 5;  // start,cmd,crc[2],end
        }
        break;
    }

    // Increment data length by one because offset needs to take into account 
    // start byte.  msgDataLength did not take this into account.
    msgDataLength++;
    // Add start byte, crc16, end byte to end of what is current in the header
    mcData.txHeaderP[msgDataLength++] = ((mcData.crc >> 8) & 0xff);
    mcData.txHeaderP[msgDataLength++] = mcData.crc & 0xff;
    mcData.txHeaderP[msgDataLength++] = MODEM_CMD_END_BYTE;
    mcData.txHeaderLength = msgDataLength; 

    // Init ISR parameters and enable ISR's to start the modem transaction.
    mcData.retryCount = 0;
    mcData.msgTxRxFailed = false;
    modemCmdIsrRestart();

    return true;
}

/**
* \brief Read the last response from the modem.  This represents 
*        the data received as a result of the last completed
*        tx/rx transaction.
* \ingroup PUBLIC_API
*/
void modemCmd_read(modemCmdReadData_t *readDataP) {
    readDataP->dataP = mcData.rxBufP;
    readDataP->lengthInBytes = mcData.rxIsrDataIndex;
    readDataP->valid = !mcData.msgTxRxFailed;
    readDataP->modemCmdId = mcData.modemCmdId;
}

/**
* \brief Return true if a modem msg tx/rx transaction is in 
*        process.
* \ingroup PUBLIC_API
* 
* @return bool True if busy.
*/
bool modemCmd_isBusy(void) {
    return mcData.busy;
}

/**
* \brief Return the status of the last modem msg rx/tx attempt.
* \ingroup PUBLIC_API
* 
* @return bool True if error occurred
*/
bool modemCmd_isError(void) {
    return mcData.msgTxRxFailed;
}

/*************************
 * Module Private Functions
 ************************/

/**
* \brief Helper function to setup parameters and enable UART 
*        hardware interrupts to start a tx/rx transaction.
*/
static void modemCmdIsrRestart(void) {
    uint8_t __attribute__((unused)) garbage;

    mcData.sendTimestamp = GET_SYSTEM_TICK();
    mcData.txIsrMsgComplete = false;
    mcData.txIsrDataIndex = 0;
    mcData.rxIsrDataIndex = 0;
    mcData.rxIsrMsgComplete = false;
    // For all non-debug cases, there is an expected response from the
    // modem.  Only if we are sending out debug data is there no response
    // expected.
    if (mcData.expectedResponseLength == 0) {
        mcData.rxIsrMsgComplete = true;
    }

    // Clear out the UART receive buffer
    garbage = UCA0RXBUF;
}

/**
* \brief Helper function to put the UART hardware and module in 
*        a quiescent state.
*/
static void modemCmdCleanup(void) {
    mcData.busy = false;
}

/**
 * \brief Helper function to check for valid values returned from
 *         the modem.  Checks for start, end response bytes and
 *         verifies CRC.
 * 
 * \return bool Returns true if rx processing was successful,
 *         false otherwise.
 */
static bool modemCmdProcessRxMsg(void) {
    bool errorOccured = false;

    // The length of the response is the rx isr buffer index
    uint8_t totalRxBytes = mcData.rxIsrDataIndex;

    // If no data back was expected, just return.
    if (mcData.expectedResponseLength == 0) {
        return true;
    }

    // Verify start byte
    if (mcData.rxBufP[0] != MODEM_RESP_START_BYTE) {
        errorOccured = true;
    }

    // Verify end byte
    if (mcData.rxBufP[(totalRxBytes - 1)] != MODEM_CMD_END_BYTE) {
        errorOccured = true;
    }

    // Verify response command type matches command sent
    if (mcData.rxBufP[1] != mcData.modemCmdId) {
        errorOccured = true;
    }

    // Verify length
    if (mcData.expectedResponseLength != totalRxBytes) {
        errorOccured = true;
    }

    if (!errorOccured) {
        // Perform CRC check on the message.
        volatile uint16_t calculatedCrc = 0;
        volatile uint16_t rxCrc = ~0;

        // The CRC of the response does not include the start byte, the crc[2] and the end byte
        uint8_t rxCrcNumBytes = totalRxBytes - 4;

        // Grab the CRC in the response
        rxCrc = mcData.rxBufP[totalRxBytes - 3];  // get MSB
        rxCrc <<= 8;
        rxCrc |= mcData.rxBufP[totalRxBytes - 2]; // get LSB

        // Calculate CRC of message
        calculatedCrc = gen_crc16(&mcData.rxBufP[1], rxCrcNumBytes);
        // compare calculated against received
        if (rxCrc != calculatedCrc) {
            errorOccured = true;
        }
    }

    return !errorOccured;
}

/*********************************************
 * UART Receive/Transmit Polling Functions
 ********************************************/

static void USCI0TX_ISR(void) {

    if (mcData.txIsrMsgComplete || !(IFG2 & UCA0TXIFG)) {
        return;
    }

    UCA0TXBUF = mcData.txHeaderP[mcData.txIsrDataIndex++];
    if (mcData.txIsrDataIndex >= mcData.txHeaderLength) {
        mcData.txIsrMsgComplete = true;
    }
}

static void USCI0RX_ISR(void) {

    bool done = false;

    if (mcData.rxIsrMsgComplete || !(IFG2 & UCA0RXIFG)) {
        return;
    }

    uint8_t rxByte = UCA0RXBUF;

    if ((mcData.rxIsrDataIndex == 0) && rxByte != MODEM_RESP_START_BYTE) {
        // Just return if we are expecting a response start byte and its not one.
        return;
    } else if (mcData.rxIsrDataIndex < ISR_BUF_SIZE) {
        // Read the data as long as there is room in the rx buffer
        mcData.rxBufP[mcData.rxIsrDataIndex++] = rxByte;
    } else {
        // Trouble - we went beyond the buffer length
        done = true;
    }

    if (mcData.rxIsrDataIndex == mcData.expectedResponseLength) {
        // If at the expected response length, we are done
        done = true;
    }

    if (done) {
        mcData.rxIsrMsgComplete = true;
    }
}
