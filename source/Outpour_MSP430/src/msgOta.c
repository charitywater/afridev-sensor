/** 
 * @file msgOta.c
 * \n Source File
 * \n Outpour MSP430 Firmware
 * 
 * \brief Manage the details of processing OTA messages.  Runs 
 * the ota state machine to retrieve OTA messages from the modem
 * and process them. 
 *  
 */
#include "outpour.h"

/***************************
 * Module Data Definitions
 **************************/

/**
 * \typedef otaState_t
 * \brief Specify the states for retrieving an OTA msg from the 
 *        modem.
 */
typedef enum otaState_e {
    OTA_STATE_IDLE,
    OTA_STATE_SEND_OTA_CMD_PHASE0,
    OTA_STATE_OTA_CMD_PHASE0_WAIT,
    OTA_STATE_PROCESS_OTA_CMD_PHASE0,
    OTA_STATE_SEND_OTA_CMD_PHASE1,
    OTA_STATE_OTA_CMD_PHASE1_WAIT,
    OTA_STATE_PROCESS_OTA_CMD_PHASE1,
    OTA_STATE_SEND_DELETE_OTA_CMD,
    OTA_STATE_DELETE_OTA_CMD_WAIT,
    OTA_STATE_SEND_OTA_ACK,
    OTA_STATE_SEND_OTA_ACK_WAIT,
    OTA_STATE_DONE,
} otaState_t;

/**
 * \typedef otaData_t
 * \brief Define a container to store data specific to 
 *        retrieving OTA messages from the modem.
 */
typedef struct otaData_s {
    bool active;                   /**< Identifies that OTA message processing is in progress  */
    otaState_t otaState;           /**< current state */
    uint8_t totalMsgsProcessed;    /**< count of messages processed per session */
    modemCmdWriteData_t cmdWrite;  /**< A pointer to a modem write cmd bject */
    bool fwUpgradeMessageReceived; /**< Identify the special fw upgrade message */
    uint8_t gmtBinSecondsOffset;   /**< value from gmt update msg candidate */
    uint8_t gmtBinMinutesOffset;   /**< value from gmt update msg candidate */
    uint8_t gmtBinHoursOffset;     /**< value from gmt update msg candidate */
    uint16_t gmtBinDaysOffset;     /**< value from gmt update msg candidate */
    uint16_t lastGmtUpdateMsgId;   /**< value from gmt update msg candidate */
    bool gmtTimeUpdateCandidate;   /**< Flag to indicate there is a GMT update candidate */
    bool gmtTimeHasBeenUpdated;    /**< Flag to indicate GMT time has been updated */
} otaData_t;

/****************************
 * Module Data Declarations
 ***************************/

/**
* \var otaData
* \brief Declare a ota message manager object.
*/
// static
otaData_t otaData;

/*************************
 * Module Prototypes
 ************************/
static void otaMsgMgr_stateMachine(void);
static bool otaMsgMgr_processOtaMsg(void);
static uint8_t otaMsgMgr_getOtaLength(void);
static bool otaMsgMgr_processGmtClocksetPart1(otaResponse_t *otaRespP);
static void otaMsgMgr_processGmtClocksetPart2(void);
static bool otaMsgMgr_processLocalOffset(otaResponse_t *otaRespP);
static bool otaMsgMgr_processResetData(otaResponse_t *otaRespP);
static bool otaMsgMgr_processResetRedFlag(otaResponse_t *otaRespP);
static bool otaMsgMgr_processActivateDevice(otaResponse_t *otaRespP);
static bool otaMsgMgr_processSilenceDevice(otaResponse_t *otaRespP);
static bool otaMsgMgr_processFirmwareUpgrade(otaResponse_t *otaRespP);
static bool otaMsgMgr_processResetDevice(otaResponse_t *otaRespP);
static void sendPhase0_OtaCommand(void);
static void sendPhase1_OtaCommand(uint8_t lengthInBytes);
static void sendDelete_OtaCommand(void);

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
    otaData.otaState = OTA_STATE_IDLE;
}

/**
* \brief Kick-off the ota state machine processing for 
*        retrieving OTA messages from the modem. The state
*        machine will retrieve messages one-by-one from the
*        modem and process them.  Once started here, the state
*        machine is executed repeatedly from the exec routine
*        until all OTA messages have been processed.
* \ingroup PUBLIC_API
*/
void otaMsgMgr_getAndProcessOtaMsgs(void) {
    otaData.active = true;
    otaData.otaState = OTA_STATE_SEND_OTA_CMD_PHASE0;
    otaData.totalMsgsProcessed = 0;
    otaData.lastGmtUpdateMsgId = 0;
    otaData.gmtTimeUpdateCandidate = false;
    otaMsgMgr_stateMachine();
}

/**
* \brief Stop any current ota message processing.
* \ingroup PUBLIC_API
*/
void otaMsgMgr_stopOtaProcessing(void) {
    if (otaData.active) {
        modemMgr_stopModemCmdBatch();
        otaData.active = false;
        // If there is a GMT update candidate, apply it now.
        if (otaData.gmtTimeUpdateCandidate) {
            otaMsgMgr_processGmtClocksetPart2();
        }
    }
    otaData.otaState = OTA_STATE_IDLE;
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

/*************************
 * Module Private Functions
 ************************/

/**
* \brief Helper function to send a "reconnaissance" get incoming
*        partial cmd to the modem.  The payloadLength is set to
*        zero, as we are just interested in finding out how big
*        the message is before requesting the full message.
*/
static void sendPhase0_OtaCommand(void) {
    memset(&otaData.cmdWrite, 0, sizeof(modemCmdWriteData_t));
    otaData.cmdWrite.cmd = M_COMMAND_GET_INCOMING_PARTIAL;
    otaData.cmdWrite.payloadLength = 0;
    otaData.cmdWrite.payloadOffset = 0;
    modemMgr_sendModemCmdBatch(&otaData.cmdWrite);
}

/**
* \brief Helper function to send a get incoming parital cmd to 
*        the modem to retrieve the OTA Messsage.
* 
* @param lengthInBytes Pre-determined length in bytes of the OTA 
*                      message as found from the reconnaissance
*                      get incoming partial message previously sent.
*/
static void sendPhase1_OtaCommand(uint8_t lengthInBytes) {
    otaData.totalMsgsProcessed++;
    memset(&otaData.cmdWrite, 0, sizeof(modemCmdWriteData_t));
    otaData.cmdWrite.cmd = M_COMMAND_GET_INCOMING_PARTIAL;
    otaData.cmdWrite.payloadLength = lengthInBytes;
    otaData.cmdWrite.payloadOffset = 0;
    modemMgr_sendModemCmdBatch(&otaData.cmdWrite);
}

/**
* \brief Helper function to send a delete partial command to the 
*        modem.
* 
*/
static void sendDelete_OtaCommand(void) {
    memset(&otaData.cmdWrite, 0, sizeof(modemCmdWriteData_t));
    otaData.cmdWrite.cmd = M_COMMAND_DELETE_INCOMING;
    modemMgr_sendModemCmdBatch(&otaData.cmdWrite);
}

static void otaMsgMgr_stateMachine(void) {
    uint8_t otaMsgByteLength = 0;
    bool continue_processing = false;
    do {
        continue_processing = false;
        switch (otaData.otaState) {
        case OTA_STATE_IDLE:
            break;

            /** 
             * PHASE 0 - send a "reconnaissance" get incoming partial cmd to the modem.
             * The payloadLength is set to zero, as we are just interested in finding out
             *  how big the message is before requesting the full message.
             */
        case OTA_STATE_SEND_OTA_CMD_PHASE0:
            sendPhase0_OtaCommand();
            otaData.otaState = OTA_STATE_OTA_CMD_PHASE0_WAIT;
            break;
        case OTA_STATE_OTA_CMD_PHASE0_WAIT:
            if (modemMgr_isModemCmdError()) {
                otaData.otaState = OTA_STATE_SEND_DELETE_OTA_CMD;
                continue_processing = true;
            } else if (modemMgr_isModemCmdComplete()) {
                otaData.otaState = OTA_STATE_PROCESS_OTA_CMD_PHASE0;
                continue_processing = true;
            }
            break;
        case OTA_STATE_PROCESS_OTA_CMD_PHASE0:
            otaMsgByteLength = otaMsgMgr_getOtaLength();
            otaData.otaState = otaMsgByteLength ?
                OTA_STATE_SEND_OTA_CMD_PHASE1 : OTA_STATE_SEND_DELETE_OTA_CMD;
            continue_processing = true;
            break;

            /**
             * PHASE 1 - send an incoming partial cmd to the modem to retrieve
             * the OTA message.  The length was previously determined in phase 0.
             */
        case OTA_STATE_SEND_OTA_CMD_PHASE1:
            sendPhase1_OtaCommand(otaMsgByteLength);
            otaData.otaState = OTA_STATE_OTA_CMD_PHASE1_WAIT;
            break;
        case OTA_STATE_OTA_CMD_PHASE1_WAIT:
            if (modemMgr_isModemCmdError()) {
                otaData.otaState = OTA_STATE_SEND_DELETE_OTA_CMD;
                continue_processing = true;
            } else if (modemMgr_isModemCmdComplete()) {
                otaData.otaState = OTA_STATE_PROCESS_OTA_CMD_PHASE1;
                continue_processing = true;
            }
            break;
        case OTA_STATE_PROCESS_OTA_CMD_PHASE1:
            {
                bool otaMsgSuccess = otaMsgMgr_processOtaMsg();
                otaData.otaState = otaMsgSuccess ?
                    OTA_STATE_SEND_OTA_ACK : OTA_STATE_SEND_DELETE_OTA_CMD;
                continue_processing = true;
            }
            break;

            /**
             * Send the OTA Reply
             */
        case OTA_STATE_SEND_OTA_ACK:
            modemMgr_sendModemCmdBatch(&otaData.cmdWrite);
            otaData.otaState = OTA_STATE_SEND_OTA_ACK_WAIT;
            break;
        case OTA_STATE_SEND_OTA_ACK_WAIT:
            if (modemMgr_isModemCmdComplete() || modemMgr_isModemCmdError()) {
                // For all messages except the firmware upgrade message,
                // move to delete the message from the modem.
                if (otaData.fwUpgradeMessageReceived) {
                    otaData.otaState = OTA_STATE_DONE;
                } else {
                    otaData.otaState = OTA_STATE_SEND_DELETE_OTA_CMD;
                }
                continue_processing = true;
            }
            break;

            /**
             * Delete the OTA Command
             */
        case OTA_STATE_SEND_DELETE_OTA_CMD:
            sendDelete_OtaCommand();
            otaData.otaState = OTA_STATE_DELETE_OTA_CMD_WAIT;
            break;
        case OTA_STATE_DELETE_OTA_CMD_WAIT:
            if (modemMgr_isModemCmdError()) {
                otaData.otaState = OTA_STATE_DONE;
            } else if (modemMgr_isModemCmdComplete()) {
                // If there is another OTA message, it should be processed
                // unless we have reached the limit (for protection).
                if (modemMgr_getNumOtaMsgsPending() && (otaData.totalMsgsProcessed < 50)) {
                    otaData.otaState = OTA_STATE_SEND_OTA_CMD_PHASE0;
                } else {
                    otaData.otaState = OTA_STATE_DONE;
                }
                continue_processing = true;
            }
            break;

        case OTA_STATE_DONE:
            otaData.active = false;
            // If there is a GMT update candidate, apply it now.
            if (otaData.gmtTimeUpdateCandidate) {
                otaMsgMgr_processGmtClocksetPart2();
            }
            otaData.otaState = OTA_STATE_IDLE;
            break;
        }
    } while (continue_processing);
}

/**
* \brief Check and save information from a GMT Clock Set 
*        message.  We only allow one GMT update to occur in the
*        system. If the GMT clock has already been updated,
*        ignore message. If GMT clock has not been updated yet,
*        then store the information to apply later. It is legal
*        to receive multiple GMT time updates messages during
*        one OTA processing session. After all OTA messages for
*        this OTA processing session has been processed, then
*        the newest GMT time update will be applied. We
*        determine newest by looking at the msgId. The higher
*        msgId takes precedence over lower msgId numbers.
* 
* @param otaRespP Pointer to the response data and other info
*                 received from the modem.
*
* @return bool True if successful
*/
static bool otaMsgMgr_processGmtClocksetPart1(otaResponse_t *otaRespP) {
    // Ignore message if a GMT update has already been applied to the system.
    if (!otaData.gmtTimeHasBeenUpdated) {
        // Get the 16 bit msgId from the message.
        uint16_t msgId = (otaRespP->buf[1] << 8) | otaRespP->buf[2];
        // Check if this msgId is newer or equal to current candidate
        if (!otaData.gmtTimeUpdateCandidate || (msgId>=otaData.lastGmtUpdateMsgId)) {
            // We have a new GMT update candidate.  Save the values to 
            // apply later.
            // Note - the times are in binary/hex values
            otaData.gmtBinSecondsOffset = otaRespP->buf[3];
            otaData.gmtBinMinutesOffset = otaRespP->buf[4];
            otaData.gmtBinHoursOffset   = otaRespP->buf[5];
            otaData.gmtBinDaysOffset   = (otaRespP->buf[6] << 8) + otaRespP->buf[7];
            otaData.lastGmtUpdateMsgId = msgId;
            otaData.gmtTimeUpdateCandidate = true;
        }
    }

    return true;
}

/**
* \brief Apply a GMT Clock update.  Sets the TI Real Time Clock
*        by incrementing the time by the values received in the
*        message.  Only applied if a new candidate is available.
*/
static void otaMsgMgr_processGmtClocksetPart2(void) {
    // Only apply the GMT update if a new candidate has been received and a 
    // GMT update was not ever previously applied.
    if (!otaData.gmtTimeHasBeenUpdated && otaData.gmtTimeUpdateCandidate) {

        // Note - the times are in binary/hex values
        uint16_t i;
        uint16_t mask;

        // Disable the timer interrupt while we are updating the time.
        mask = getAndDisableSysTimerInterrupt();
        for (i = 0; i < otaData.gmtBinSecondsOffset; i++) {
            incrementSeconds();
        }
        for (i = 0; i < otaData.gmtBinMinutesOffset; i++) {
            incrementMinutes();
        }
        for (i = 0; i < otaData.gmtBinHoursOffset; i++) {
            incrementHours();
        }
        for (i = 0; i < otaData.gmtBinDaysOffset; i++) {
            incrementDays();
        }
        // Restore timer interrupt
        restoreSysTimerInterrupt(mask);

        otaData.gmtTimeHasBeenUpdated = true;
        otaData.gmtTimeUpdateCandidate = false;
    }
}

/**
* \brief Process the Local Time Offset command.  Identifies when
*        a new storage data should start.
* 
* @param otaRespP Pointer to the response data and other info
*                 received from the modem.
*
* @return bool True if successful
*/
static bool otaMsgMgr_processLocalOffset(otaResponse_t *otaRespP) {
    // Note - these values are sent as bcd values
    uint8_t bcdSecond = otaRespP->buf[3];
    uint8_t bcdMinute = otaRespP->buf[4];
    uint8_t bcdHour24 = otaRespP->buf[5];
    storageMgr_setStorageAlignmentTime(bcdSecond, bcdMinute, bcdHour24);
    return true;
}

/**
* \brief Process the Reset Data OTA command.  
* 
* @param otaRespP Pointer to the response data and other info
*                 received from the modem.
*
* @return bool True if successful
*/
static bool otaMsgMgr_processResetData(otaResponse_t *otaRespP) {
    storageMgr_overrideUnitActivation(false);
    storageMgr_resetRedFlagAndMap();
    storageMgr_resetWeeklyLogs();
    return true;
}

/**
* \brief Process the Reset Red Flag OTA command.  
* 
* @param otaRespP Pointer to the response data and other info
*                 received from the modem.
*
* @return bool True if successful
*/
static bool otaMsgMgr_processResetRedFlag(otaResponse_t *otaRespP) {
    storageMgr_resetRedFlag();
    return true;
}

/**
* \brief Process Activate Device OTA command.  
* 
* @param otaRespP Pointer to the response data and other info
*                 received from the modem.
*
* @return bool True if successful
*/
static bool otaMsgMgr_processActivateDevice(otaResponse_t *otaRespP) {
    storageMgr_overrideUnitActivation(true);
    return true;
}

/**
* \brief Process De-Activate Device OTA command.  
* 
* @param otaRespP Pointer to the response data and other info
*                 received from the modem.
*
* @return bool True if successful
*/
static bool otaMsgMgr_processSilenceDevice(otaResponse_t *otaRespP) {
    storageMgr_overrideUnitActivation(false);
    return true;
}

/**
* \brief Process Firmware Upgrade OTA command.  
* 
* @param otaRespP Pointer to the response data and other info
*                 received from the modem.
*
* @return bool True if successful
*/
static bool otaMsgMgr_processFirmwareUpgrade(otaResponse_t *otaRespP) {        // Verify Keys of the firmware upgrade message
    bool status = false;
    uint8_t *bufP = otaRespP->buf;
    // Verify the firmware upgrade keys in the message.
    if ((bufP[3] == FLASH_UPGRADE_KEY1) &&
        (bufP[4] == FLASH_UPGRADE_KEY2) &&
        (bufP[5] == FLASH_UPGRADE_KEY3) &&
        (bufP[6] == FLASH_UPGRADE_KEY4)) {
        // To process the firmware upgrade message, we want to boot into the bootloader.
        // So set up the system for a reboot.  We need valid reboot keys in the
        // buffer for the reboot function to allow a reboot to start.
        bufP[3] = REBOOT_KEY1;
        bufP[4] = REBOOT_KEY2;
        bufP[5] = REBOOT_KEY3;
        bufP[6] = REBOOT_KEY4;
        sysExec_startRebootCountdown(&otaRespP->buf[3]);
        otaData.fwUpgradeMessageReceived = true;
        status = true;
    }
    return status;
}

/**
* \brief Process Reset Device OTA command.  
* 
* @param otaRespP Pointer to the response data and other info
*                 received from the modem.
*
* @return bool True if successful
*/
static bool otaMsgMgr_processResetDevice(otaResponse_t *otaRespP) {
    return sysExec_startRebootCountdown(&otaRespP->buf[3]);
}

/**
* \brief Get the length of an OTA message. 
*
* @return uint8_t length in bytes.
*/
static uint8_t otaMsgMgr_getOtaLength(void) {
    // Get the object that contains the OTA message data and info
    otaResponse_t *otaRespP = modemMgr_getLastOtaResponse();
    uint8_t bytes = otaRespP->remainingInBytes;
    // The only message that is longer than the OTA buffer size
    // is the upgrade message.  For that, we only need to identify
    // that we received it.  It will be processed in the bootloader.
    if (bytes > OTA_PAYLOAD_BUF_LENGTH) {
        bytes = 16;
    }
    return bytes;
}

/**
* \brief Process OTA commands.  
*
* @return bool True if successful
*/
static bool otaMsgMgr_processOtaMsg(void) {
    bool success = false;
    // Get the buffer that contains the OTA message data
    otaResponse_t *otaRespP = modemMgr_getLastOtaResponse();
    // First byte is the OTA opcode
    volatile OtaOpcode_t opcode = (OtaOpcode_t)otaRespP->buf[0];

    switch (opcode) {
    case OTA_OPCODE_GMT_CLOCKSET:
        success = otaMsgMgr_processGmtClocksetPart1(otaRespP);
        break;
    case OTA_OPCODE_LOCAL_OFFSET:
        success = otaMsgMgr_processLocalOffset(otaRespP);
        break;
    case OTA_OPCODE_RESET_ALL:
        success = otaMsgMgr_processResetData(otaRespP);
        break;
    case OTA_OPCODE_RESET_RED_FLAG:
        success = otaMsgMgr_processResetRedFlag(otaRespP);
        break;
    case OTA_OPCODE_ACTIVATE_DEVICE:
        success = otaMsgMgr_processActivateDevice(otaRespP);
        break;
    case OTA_OPCODE_SILENCE_DEVICE:
        success = otaMsgMgr_processSilenceDevice(otaRespP);
        break;
    case OTA_OPCODE_FIRMWARE_UPGRADE:
        success = otaMsgMgr_processFirmwareUpgrade(otaRespP);
        break;
    case OTA_OPCODE_RESET_DEVICE:
        success = otaMsgMgr_processResetDevice(otaRespP);
        break;
    default:
        break;
    }

    if (success) {
        // Get the msgID from the OTA buffer
        volatile uint8_t msgId0 = otaRespP->buf[1];
        volatile uint8_t msgId1 = otaRespP->buf[2];

        // prepare OTA response message
        // Get the shared buffer (we borrow the ota buffer)
        uint8_t *bufP = modemMgr_getSharedBuffer();

        // Zero the OTA response buffer
        memset (bufP, 0, OTA_PAYLOAD_BUF_LENGTH);

        // Fill in the buffer with the standard message header
        uint8_t index = storageMgr_prepareMsgHeader(bufP);

        // Always send 32 bytes back beyond the header - even if some not used.
        // Allows for future additions to return other data
        uint8_t returnLength = index + 32;

        // Copy the opcode we received into the buffer
        bufP[index++] = opcode;
        // Copy the message ID we received into the buffer
        bufP[index++] = msgId0;
        bufP[index++] = msgId1;

        // Prepare the command for the batch write operation
        memset(&otaData.cmdWrite, 0, sizeof(modemCmdWriteData_t));
        otaData.cmdWrite.cmd = M_COMMAND_SEND_DATA;
        otaData.cmdWrite.payloadMsgId = MSG_TYPE_OTAREPLY;
        otaData.cmdWrite.payloadP = bufP;
        otaData.cmdWrite.payloadLength = returnLength;
    }

    return success;
}

