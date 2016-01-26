/** 
 * @file msgOta.c
 * \n Source File
 * \n Outpour MSP430 Bootloader Firmware
 * 
 * \brief Manage the details of processing OTA messages.  Runs 
 * the ota state machine to retrieve OTA messages from the modem
 * and process them.  This module has been customized around the 
 * firmware upgrade OTA processing. 
 *  
 */
#include "outpour.h"

/***************************
 * Module Data Definitions
 **************************/

/**
 * \def WAIT_FOR_LINK_UP_TIME_IN_SECONDS
 * \brief Specify how long to wait for the modem to connect to 
 *        the network.
 */
#define WAIT_FOR_LINK_UP_TIME_IN_SECONDS ((uint16_t)60*10*(SYS_TICKS_PER_SECOND))

/**
 * \def OTA_UPDATE_MSG_HEADER_SIZE
 * \brief Specify information about the upgrade message.  The 
 *        header portion of the message contains 8 bytes.
 */
#define OTA_UPDATE_MSG_HEADER_SIZE ((uint8_t)8)
/**
 * \def OTA_UPDATE_SECTION_HEADER_SIZE
 * \brief Specify information about the upgrade message. After 
 *        the header comes a section description header.  This
 *        contains information on a continuous section of code
 *        that needs to be programmed into flash.
 */
#define OTA_UPDATE_SECTION_HEADER_SIZE ((uint8_t)8)
/**
 * \def FLASH_UPGRADE_SECTION_START
 * \brief Specify information about the upgrade message.  The 
 *        section description begins with an 0xA5.
 */
#define FLASH_UPGRADE_SECTION_START ((uint8_t)0xA5)

/**
 * \typedef modemBatchCmdType_t
 * \brief Specify the modem batch command to prepare.
 */
typedef enum modemBatchCmdType_e {
    MODEM_BATCH_CMD_STATUS_ONLY,
    MODEM_BATCH_CMD_SOS,
    MODEM_BATCH_CMD_GET_OTA_PARTIAL,
    MODEM_BATCH_CMD_DELETE_MESSAGE
} modemBatchCmdType_t;

/**
 * \typedef otaState_t
 * \brief Specify the states for retrieving an OTA msg from the 
 *        modem.
 */
typedef enum otaState_e {
    OTA_STATE_IDLE,
    OTA_STATE_GRAB,
    OTA_STATE_WAIT_FOR_MODEM_UP,
    OTA_STATE_SEND_OTA_CMD_PHASE0,
    OTA_STATE_OTA_CMD_PHASE0_WAIT,
    OTA_STATE_WAIT_FOR_LINK,
    OTA_STATE_PROCESS_OTA_CMD_PHASE0,
    OTA_STATE_SEND_OTA_CMD_PHASE1,
    OTA_STATE_OTA_CMD_PHASE1_WAIT,
    OTA_STATE_PROCESS_OTA_CMD_PHASE1,
    OTA_STATE_SEND_DELETE_OTA_CMD,
    OTA_STATE_DELETE_OTA_CMD_WAIT,
    OTA_STATE_DONE,
} otaState_t;

/**
 * \typedef otaFlashState_t
 * \brief Specify the states for writing and erasing flash with 
 *        new firmware.
 */
typedef enum otaFlashState_e {
    OTA_FLASH_STATE_START,
    OTA_FLASH_STATE_GET_SECTION_INFO,
    OTA_FLASH_STATE_WRITE_SECTION_DATA,
} otaFlashState_t;

/**
 * \typedef otaData_t
 * \brief Define a container to store data specific to 
 *        retrieving OTA messages from the modem.
 */
typedef struct otaData_s {
    bool active;                     /**< marks the water msg manager as busy */
    bool sos;                        /**< specifies that we are in SOS mode */
    otaState_t otaState;             /**< current state */
    modemCmdWriteData_t cmdWrite;    /**< A pointer to a modem write cmd object */
    uint8_t modemRequestLength;      /**< Specify how much data to get from the modem */
    uint16_t modemRequestOffset;     /**< Specify how much data to get from the modem */
    otaFlashState_t otaFlashState;   /**< current state of flash state machine */
    uint8_t totalSections;           /**< Total number of code sections in the fw update message */
    uint8_t nextSectionNumber;       /**< The next code section to process of the fw update message */
    uint16_t sectionStartAddrP;      /**< Current upgrade section start address */
    uint16_t sectionDataLength;      /**< Current upgrade section total data length */
    uint16_t sectionCrc16;           /**< Current section CRC16 value */
    uint16_t sectionDataRemaining;   /**< Count in bytes of section data remaining to retrieve from modem */
    uint8_t *sectionWriteAddrP;      /**< Where in flash to write the data */
    fwUpdateResult_t fwUpdateResult; /**< result for last flash state machine processing */
} otaData_t;

/****************************
 * Module Data Declarations
 ***************************/

/**
* \var otaData
* \brief Declare a data object to "house" data for this module.
*/
static otaData_t otaData;

/*************************
 * Module Prototypes
 ************************/
static void otaMsgMgr_stateMachine(void);
static void otaMsgMgr_processOtaMsg(void);
static bool otaMsgMgr_checkForOta(void);
static void otaUpgrade_start(void);
static void otaUpgrade_getSectionInfo(void);
static void otaUpgrade_eraseSection(void);
static void otaUpgrade_writeSectionData(void);
static void otaUpgrade_verifySection(void);
static void erase_app_reset_vector(void);

/***************************
 * Module Public Functions
 **************************/

/**
* \brief Exec routine should be called as part of the main 
*        processing loop. Drives the ota message state machine.
*        Will only run the state machine if activated to do so
*        via the otaMsgMgr_getAndProcessOtaMsgs function.
* \ingroup EXEC_ROUTINE
*/
void otaMsgMgr_exec(void) {
    if (otaData.active) {
        otaMsgMgr_stateMachine();
    }
}

/**
* \brief Call once as system startup to initialize the ota 
*        message manager object.
* \ingroup PUBLIC_API
*/
void otaMsgMgr_init(void) {
    memset(&otaData, 0, sizeof(otaData_t));
}

/**
* \brief Kick-off the ota state machine processing for 
*        retrieving OTA messages from the modem. The state
*        machine will retrieve messages one-by-one from the
*        modem and process them.  Customized to process the
*        Firmware upgrade message.  All other messages will be
*        deleted.
* \ingroup PUBLIC_API
*
* @param sos true/false flag to specify whether an SOS message
*            should be sent as part of the OTA processing.
*/
void otaMsgMgr_getAndProcessOtaMsgs(bool sos) {
    // Init variables needed for upgrade session
    otaData.active = true;
    otaData.otaState = OTA_STATE_GRAB;
    otaData.sos = sos;
    otaData.fwUpdateResult = RESULT_NO_FWUPGRADE_PERFORMED;
    // Run the OTA state machine
    otaMsgMgr_stateMachine();
}

/**
* \brief Poll to identify if the ota message processing is 
*        complete.
* \ingroup PUBLIC_API
* 
* @return bool True is ota message processing is not active.
*/
bool otaMsgMgr_isOtaProcessingDone(void) {
    return !otaData.active;
}

/**
* \brief Return the status of the last fw update message check 
*        and fw update attempt.
* \ingroup PUBLIC_API
* 
* @return fwUpdateResult_t  The status of the last fw update 
*         check for a modem update message, and the fw update
*         result if a fw update message was available from the
*         modem.
*/
fwUpdateResult_t otaMsgMgr_getFwUpdateResult(void) {
    return otaData.fwUpdateResult;
}

/*************************
 * Module Private Functions
 ************************/

static void sendModemBatchCmd(modemBatchCmdType_t cmdType) {
    memset(&otaData.cmdWrite, 0, sizeof(modemCmdWriteData_t));
    if (cmdType == MODEM_BATCH_CMD_STATUS_ONLY) {
        otaData.cmdWrite.statusOnly = true;
    } else if (cmdType == MODEM_BATCH_CMD_GET_OTA_PARTIAL) {
        otaData.cmdWrite.cmd = M_COMMAND_GET_INCOMING_PARTIAL;
        otaData.cmdWrite.payloadLength = otaData.modemRequestLength;
        otaData.cmdWrite.payloadOffset = otaData.modemRequestOffset;
    } else if (cmdType == MODEM_BATCH_CMD_SOS) {
        otaData.cmdWrite.cmd = M_COMMAND_SEND_SOS;
        otaData.cmdWrite.payloadMsgId = MSG_TYPE_SOS;
    } else if (cmdType == MODEM_BATCH_CMD_DELETE_MESSAGE) {
        otaData.cmdWrite.cmd = M_COMMAND_DELETE_INCOMING;
    }
    modemMgr_sendModemCmdBatch(&otaData.cmdWrite);
}

static void otaMsgMgr_stateMachine(void) {
    switch (otaData.otaState) {
    case OTA_STATE_IDLE:
        break;

    case OTA_STATE_GRAB:
        if (modemMgr_grab()) {
            otaData.otaState = OTA_STATE_WAIT_FOR_MODEM_UP;
        }
        break;

    case OTA_STATE_WAIT_FOR_MODEM_UP:
        if (modemMgr_isModemUp()) {
            otaData.otaState = OTA_STATE_SEND_OTA_CMD_PHASE0;
        }
        break;

        /** 
         * PHASE 0 - This phase is used to determine if the modem has a 
         * message available to retrieve.  For this case, we send only a
         * status request batch command.  But if the SOS flag is set, we
         * also send an SOS message to the cloud as well as retrieve 
         * status information.
         */
    case OTA_STATE_SEND_OTA_CMD_PHASE0:
        if (otaData.sos) {
            sendModemBatchCmd(MODEM_BATCH_CMD_SOS);
        } else {
            sendModemBatchCmd(MODEM_BATCH_CMD_STATUS_ONLY);
        }
        otaData.otaState = OTA_STATE_OTA_CMD_PHASE0_WAIT;
        break;

    case OTA_STATE_OTA_CMD_PHASE0_WAIT:
        if (modemMgr_isModemCmdError()) {
            // If there was a modem error, we abort.
            otaData.fwUpdateResult = RESULT_DONE_ERROR;

            // TODO - We don't really want to delete the message yet.
            // But because we have no more flash memory to put logic,
            // we can't put any more recovery here.  The problem is that we
            // can't let messages build up in the modem or else that is a certain
            // brick situation.  We would like to retry one more time before
            // deleting the message, but we need more memory to put that logic in.
            // So for now, we need to delete the message if we get a modem error.
            otaData.otaState = OTA_STATE_SEND_DELETE_OTA_CMD;

        } else if (modemMgr_isModemCmdComplete()) {
            // If we sent an SOS message, we want to wait until the modem is
            // connected to the network (i.e. the link is up).  Otherwise we
            // don't care about connecting to the network, as we only wanted to
            // check if the modem has a message stored internally for us to
            // retrieve.
            if (otaData.sos) {
                otaData.otaState = OTA_STATE_WAIT_FOR_LINK;
            } else {
                otaData.otaState = OTA_STATE_PROCESS_OTA_CMD_PHASE0;
            }
        }
        break;

    case OTA_STATE_WAIT_FOR_LINK:
        // Waiting for network link to come up
        if (modemMgr_isLinkUp() ||
            modemMgr_isLinkUpError() ||
            (modemLink_getModemUpTimeInSysTicks() > WAIT_FOR_LINK_UP_TIME_IN_SECONDS)) {
            // Done waiting.  Move to retrieving any OTA Messages.
            otaData.otaState = OTA_STATE_PROCESS_OTA_CMD_PHASE0;
        } else {
            // While waiting for the modem link to come up,
            // retrieve status only from modem.
            sendModemBatchCmd(MODEM_BATCH_CMD_STATUS_ONLY);
            otaData.otaState = OTA_STATE_OTA_CMD_PHASE0_WAIT;
        }
        break;

    case OTA_STATE_PROCESS_OTA_CMD_PHASE0:
        // Check if there is an OTA message available from the modem.
        if (otaMsgMgr_checkForOta()) {
            otaData.otaFlashState = OTA_FLASH_STATE_START;
            // Init variables so we grab the right amount of data as we
            // head into retrieving the message from the modem.
            otaData.modemRequestLength = OTA_UPDATE_MSG_HEADER_SIZE;
            otaData.modemRequestOffset = 0;
            otaData.otaState = OTA_STATE_SEND_OTA_CMD_PHASE1;
        } else {
            // No messages available from the modem.  We are done.
            otaData.otaState = OTA_STATE_DONE;
        }
        break;

        /**
         * PHASE 1 - send an incoming partial cmd to the modem to retrieve
         * the first bytes of the OTA message.  We are only interested 
         * in retrieving the header portion of the message - enough to 
         * identify the type of message that it is. 
         */
    case OTA_STATE_SEND_OTA_CMD_PHASE1:
        sendModemBatchCmd(MODEM_BATCH_CMD_GET_OTA_PARTIAL);
        // Track how many bytes of the message we have requested.
        otaData.modemRequestOffset += otaData.modemRequestLength;
        otaData.otaState = OTA_STATE_OTA_CMD_PHASE1_WAIT;
        break;

    case OTA_STATE_OTA_CMD_PHASE1_WAIT:
        if (modemMgr_isModemCmdError()) {
            // If there was a modem error, we abort.
            otaData.fwUpdateResult = RESULT_DONE_ERROR;

            // TODO - We don't really want to delete the message yet.
            // But because we have no more flash memory to put logic,
            // we can't put any more recovery here.  The problem is that we
            // can't let messages build up in the modem or else that is a certain
            // brick situation.  We would like to retry one more time before
            // deleting the message, but we need more memory to put that logic in.
            // So for now, we need to delete the message if we get a modem error.
            otaData.otaState = OTA_STATE_SEND_DELETE_OTA_CMD;

        } else if (modemMgr_isModemCmdComplete()) {
            // Move to processing the data retrieved from the modem.
            otaData.otaState = OTA_STATE_PROCESS_OTA_CMD_PHASE1;
        }
        break;

    case OTA_STATE_PROCESS_OTA_CMD_PHASE1:
        {
            // Process the data retrieved from the modem.
            otaMsgMgr_processOtaMsg();

            // Determine next action based on the results of the processing.
            if (otaData.fwUpdateResult == RESULT_GET_MORE_DATA) {
                // Retrieve more data from the modem.
                otaData.otaState = OTA_STATE_SEND_OTA_CMD_PHASE1;
            } else if (otaData.fwUpdateResult & (RESULT_DONE_SUCCESS | RESULT_DONE_ERROR)) {
                // We are done.  Move to delete the message from the modem.
                otaData.otaState = OTA_STATE_SEND_DELETE_OTA_CMD;
            } else if (otaData.fwUpdateResult == RESULT_NO_FWUPGRADE_PERFORMED) {
                // There was no upgrade message available.
                // If we are in an SOS condition, we need to clear out any messages
                // stored in the modem other than upgrade messages.
                if (otaData.sos) {
                    otaData.otaState = OTA_STATE_SEND_DELETE_OTA_CMD;
                } else {
                    otaData.otaState = OTA_STATE_DONE;
                }
            }
        }
        break;

        /**
         * Delete the OTA Command
         */
    case OTA_STATE_SEND_DELETE_OTA_CMD:
        sendModemBatchCmd(MODEM_BATCH_CMD_DELETE_MESSAGE);
        otaData.otaState = OTA_STATE_DELETE_OTA_CMD_WAIT;
        break;

    case OTA_STATE_DELETE_OTA_CMD_WAIT:
        if (modemMgr_isModemCmdError()) {
            otaData.otaState = OTA_STATE_DONE;
        } else if (modemMgr_isModemCmdComplete()) {
            // If we are in an SOS condition we need to
            // handle all messages in the modem (i.e. delete them).
            // Otherwise we can get into a stuck situation and never recover.
            if (otaData.sos && (otaData.fwUpdateResult != RESULT_DONE_SUCCESS)) {
                otaData.otaState = OTA_STATE_PROCESS_OTA_CMD_PHASE0;
            } else {
                otaData.otaState = OTA_STATE_DONE;
            }
        }
        break;

    case OTA_STATE_DONE:
        modemMgr_release();
        otaData.active = false;
        otaData.otaState = OTA_STATE_IDLE;
        break;
    }
}

/**
* \brief Check for the presence of an OTA message. 
*
* @return bool true if an OTA message is waiting and the message
*         size is greater than OTA_UPDATE_MSG_HEADER_SIZE;
*/
static bool otaMsgMgr_checkForOta(void) {
    bool status = false;
    uint8_t numOtaMessages = modemMgr_getNumOtaMsgsPending();
    uint16_t sizeOfOtaMessages = modemMgr_getSizeOfOtaMsgsPending();
    if (numOtaMessages && sizeOfOtaMessages > OTA_UPDATE_MSG_HEADER_SIZE) {
        status = true;
    }
    return status;
}

/**
* \brief Process the OTA message.  Setup to perform the firmware 
*        update message only.  All other messages will be
*        deleted.
*/
static void otaMsgMgr_processOtaMsg(void) {
    switch (otaData.otaFlashState) {
    case OTA_FLASH_STATE_START:
        otaUpgrade_start();
        break;
    case OTA_FLASH_STATE_GET_SECTION_INFO:
        otaUpgrade_getSectionInfo();
        break;
    case OTA_FLASH_STATE_WRITE_SECTION_DATA:
        otaUpgrade_writeSectionData();
        break;
    }
}

/**
* \brief Firmware Upgrade State Machine Function
*/
static void otaUpgrade_start(void) {
    // Get the buffer that contains the OTA message data
    otaResponse_t *otaRespP = modemMgr_getLastOtaResponse();
    // First byte is the OTA opcode
    volatile OtaOpcode_t opcode = (OtaOpcode_t)otaRespP->buf[0];
    if (opcode != OTA_OPCODE_FIRMWARE_UPGRADE) {
        otaData.fwUpdateResult = RESULT_NO_FWUPGRADE_PERFORMED;
    } else {
        uint8_t *bufP = &otaRespP->buf[0];
        // Verify Keys of the firmware upgrade message
        if ((bufP[3] == FLASH_UPGRADE_KEY1) &&
            (bufP[4] == FLASH_UPGRADE_KEY2) &&
            (bufP[5] == FLASH_UPGRADE_KEY3) &&
            (bufP[6] == FLASH_UPGRADE_KEY4)) {
            otaData.totalSections = bufP[7];
            otaData.nextSectionNumber = 0;
            otaData.modemRequestLength = OTA_UPDATE_SECTION_HEADER_SIZE;
            otaData.otaFlashState = OTA_FLASH_STATE_GET_SECTION_INFO;
            otaData.fwUpdateResult = RESULT_GET_MORE_DATA;
        } else {
            otaData.fwUpdateResult = RESULT_DONE_ERROR;
        }
    }
}

/**
* \brief Firmware Upgrade State Machine Function
*/
static void otaUpgrade_getSectionInfo(void) {
    uint16_t bootFlashStartAddr = getBootFlashStartAddr();
    uint16_t appFlashStartAddr = getAppFlashStartAddr();
    uint16_t appFlashLength = getAppFlashLength();
    // Get the buffer that contains the OTA message data
    otaResponse_t *otaRespP = modemMgr_getLastOtaResponse();
    volatile uint8_t *bufP = &otaRespP->buf[0];
    uint8_t sectionStartId = *bufP++;
    uint8_t sectionNumber = *bufP++;
    // If all goes well, otaUpgrade_eraseSection will update
    // the fwUpdateResult.
    otaData.fwUpdateResult = RESULT_DONE_ERROR;
    if ((sectionStartId == FLASH_UPGRADE_SECTION_START) &&
        (sectionNumber == otaData.nextSectionNumber++)) {
        // Start Addr
        otaData.sectionStartAddrP  = (*bufP++ << 8);
        otaData.sectionStartAddrP |= *bufP++;
        otaData.sectionWriteAddrP  = (uint8_t *)otaData.sectionStartAddrP;
        // Length
        otaData.sectionDataLength  = (*bufP++ << 8);
        otaData.sectionDataLength |= *bufP++;
        otaData.sectionDataRemaining = otaData.sectionDataLength;
        // CRC
        otaData.sectionCrc16  = (*bufP++ << 8);
        otaData.sectionCrc16 |= *bufP++;

        // Verify the section parameters
        // 1. Check that the modem message has enough data to fill the section.
        // 2. Check that the section start and end address is located in app flash area
        // Otherwise we consider this a catastrophic error.

        uint16_t startBurnAddr = otaData.sectionStartAddrP;
        uint16_t endBurnAddr = otaData.sectionStartAddrP + otaData.sectionDataLength;
        if ((otaRespP->remainingInBytes >= otaData.sectionDataLength) &&
            (otaData.sectionDataLength <= appFlashLength) &&
            (startBurnAddr >= appFlashStartAddr) && 
            (startBurnAddr < bootFlashStartAddr) && 
            (endBurnAddr > appFlashStartAddr)    && 
            (endBurnAddr <= bootFlashStartAddr)) {
            // We have verified the section information.  Now erase the flash.
            otaUpgrade_eraseSection();
        }
    }
}

/**
* \brief Firmware Upgrade State Machine Function
*/
static void otaUpgrade_eraseSection(void) {
    uint8_t i;
    // Because we are going to divide the data length by 512 to
    // get the number of flash sections to erase, add 511 bytes
    // to make sure we don't miscalculate the number of sections
    // to erase by losing bits in the divide.
    uint16_t numSectors = otaData.sectionDataLength + ((uint16_t)511);
    numSectors >>= 9; // divide by 512
    uint8_t *flashSegmentAddrP = (uint8_t *)otaData.sectionStartAddrP;
    uint8_t *bootFlashStartAddrP = (uint8_t *)getBootFlashStartAddr();

    // Before we start wiping out the application, zero the app reset vector
    // so that we can never go back unless the programing completes succesfully.
    // The app reset vector is always checked for a valid range before jumping to it.
    erase_app_reset_vector();

    // Erase the flash segments
    for (i = 0; i < numSectors; i++, flashSegmentAddrP += 0x200) {
        // Check that the address falls below the bootloader flash area
        if (flashSegmentAddrP < bootFlashStartAddrP) {
            // Tickle the watchdog before erasing
            WATCHDOG_TICKLE();
            msp430Flash_erase_segment(flashSegmentAddrP);
        }
    }

    // Set up for starting the write data to flash.
    // Initialize request size from modem.
    // The maximum data we can request from the modem at one time is
    // OTA_PAYLOAD_BUF_LENGTH
    if (otaData.sectionDataRemaining > OTA_PAYLOAD_BUF_LENGTH) {
        otaData.modemRequestLength = OTA_PAYLOAD_BUF_LENGTH;
    } else {
        otaData.modemRequestLength = otaData.sectionDataRemaining;
    }

    // Setup state for the write to flash sequence.
    otaData.otaFlashState = OTA_FLASH_STATE_WRITE_SECTION_DATA;
    otaData.fwUpdateResult = RESULT_GET_MORE_DATA;
}

/**
* \brief Firmware Upgrade State Machine Function
*/
static void otaUpgrade_writeSectionData(void) {
    // Get the buffer that contains the OTA message data
    otaResponse_t *otaRespP = modemMgr_getLastOtaResponse();
    uint8_t *bufP = &otaRespP->buf[0];

    // Make sure we got data back from the modem, else its an error condition.
    if (otaRespP->lengthInBytes > 0) {

        uint8_t writeDataSize = otaRespP->lengthInBytes;
        uint8_t *bootFlashStartAddrP = (uint8_t *)getBootFlashStartAddr();

        // This should not happen but just in case, make sure
        // we did not get too much data back from the modem.
        if (otaData.sectionDataRemaining < writeDataSize) {
            writeDataSize = otaData.sectionDataRemaining;
        }

        // Write the data to flash.
        // Check that we are not going to write into the bootloader flash area.
        if ((otaData.sectionWriteAddrP + writeDataSize) <= bootFlashStartAddrP) {
            // Tickle the watchdog before erasing
            WATCHDOG_TICKLE();
            msp430Flash_write_bytes(otaData.sectionWriteAddrP, &bufP[0], writeDataSize);
        }

        // Update counters and flash pointer
        otaData.sectionDataRemaining -= writeDataSize;
        otaData.sectionWriteAddrP += writeDataSize;

        // Check if we have all the data for the section.
        // If so, call the flash verify function.
        // Otherwise, set request length for next buffer of data.
        if (otaData.sectionDataRemaining == 0x0) {
            otaUpgrade_verifySection();
        } else {
            // We need more data.
            if (otaData.sectionDataRemaining > OTA_PAYLOAD_BUF_LENGTH) {
                otaData.modemRequestLength = OTA_PAYLOAD_BUF_LENGTH;
            } else {
                otaData.modemRequestLength = otaData.sectionDataRemaining;
            }
            otaData.fwUpdateResult = RESULT_GET_MORE_DATA;
        }
    } else {
        // We did not get any data back from the modem.
        // Error condition.
        otaData.fwUpdateResult = RESULT_DONE_ERROR;
        // Zero the app reset vector so that this app image will not be used.
        erase_app_reset_vector();
    }
}

/**
* \brief Firmware Upgrade State Machine Function
*/
static void otaUpgrade_verifySection(void) {
    uint16_t calcCrc16 = gen_crc16((const unsigned char *)otaData.sectionStartAddrP, otaData.sectionDataLength);
    if (calcCrc16 == otaData.sectionCrc16) {
        // For outpour, we only can support one section due to lack of 
        // bootloader memory to support burning multiple sections.
        otaData.fwUpdateResult = RESULT_DONE_SUCCESS;

#if 0
        **** If we did support multiple sections *****
        // There could be more code sections to write.
        // Otherwise we are done.
        if (otaData.totalSections == otaData.nextSectionNumber) {
            otaData.fwUpdateResult = RESULT_DONE_SUCCESS;
        } else {
            otaData.modemRequestLength = OTA_UPDATE_SECTION_HEADER_SIZE;
            otaData.otaFlashState = OTA_FLASH_STATE_GET_SECTION_INFO;
            otaData.fwUpdateResult = RESULT_GET_MORE_DATA;
        }
#endif

    } else {
        // The CRC failed.
        // Error condition.
        otaData.fwUpdateResult = RESULT_DONE_ERROR;
        // Zero the app reset vector so that this app image will not be used.
        erase_app_reset_vector();
    }
}

/**
* \brief Firmware Upgrade Utility Function
*/
static void erase_app_reset_vector(void) {
    // Use the linker symbol to identify where to zero the application reset vector in flash.
    void (*tempP)(void) = 0;
    msp430Flash_write_bytes((uint8_t *)&_App_Reset_Vector, (uint8_t *)&tempP, sizeof(void (*)(void)));
}

