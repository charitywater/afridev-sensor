/** 
 * @file msgWater.c
 * \n Source File
 * \n Outpour MSP430 Firmware
 * 
 * \brief Manage the high level details of sending a data 
 *        message to the modem.
 */

#include "outpour.h"

/***************************
 * Module Data Definitions
 **************************/

/**
 * \def DATA_MSG_MAX_RETRIES
 * \brief Specify how many retries to attempt to send the data 
 *        msg if the network was not able to connect.
 */
#define DATA_MSG_MAX_RETRIES ((uint8_t)1)

/**
 * \def DATA_MSG_DELAY_IN_SECONDS_TILL_RETX
 * \brief Specify how long to wait to retransmit as a result of 
 *        the modem failing to connect to the network.
 * \note Max hours in a uint16_t representing seconds is 18.2 
 *       hours.  Currently set at 12 hours.
 */
#define DATA_MSG_DELAY_IN_SECONDS_TILL_RETX ((uint16_t)12*60*60)

/**
 * \typedef msgData_t
 * \brief Define a container to store data and state information
 *        to support sending a data message to the modem.
 */
typedef struct msgData_s {
    bool sendDataMsgActive;    /**< flag to mark a data message is in progress */
    bool sendDataMsgScheduled; /**< flag to mark a data message is scheduled */
    bool sendFaScheduled;      /**< flag to mark a send Final Assembly is scheduled */
    bool sendFaActive;         /**< flag to mark a send Final Assembly is in progress */
    bool sendDailyLogs;        /**< flag to indicate we are sending daily logs */
    uint8_t weeklyLogNum;      /**< which weekly log to access when retrieving daily logs */
    uint8_t retryCount;           /**< number of retries attempted */
    uint16_t secsTillTransmit; /**< time in seconds until transmit: max is 18.2 hours as 16 bit value */
    dataMsgSm_t dataMsgSm;     /**< Data message state machine object */
} msgData_t;

/****************************
 * Module Data Declarations
 ***************************/

/**
* \var wmmData
* \brief Declare the data msg object.
*/
// static
msgData_t msgData;

/*************************
 * Module Prototypes
 ************************/

/***************************
 * Module Public Functions
 **************************/

/**
* \brief Call once as system startup to initialize the data 
*        message module.
* \ingroup PUBLIC_API
*/
void dataMsgMgr_init(void) {
    memset(&msgData, 0, sizeof(msgData_t));
}

/**
* \brief Exec routine should be called once every time from the 
*        main processing loop.  Drives the data message state
*        machine if there is a data message transmit currently
*        in progress.
* \ingroup EXEC_ROUTINE
*/
void dataMsgMgr_exec(void) {

    if (msgData.sendDataMsgActive) {

        dataMsgSm_t *dataMsgSmP = &msgData.dataMsgSm;

        // If we are sending daily logs, then check if the data message state
        // machine is done sending the current daily log.  If done, then we
        // will get the next daily log, and start sending it.
        if (msgData.sendDailyLogs && dataMsgSmP->sendCmdDone) {
            uint8_t *dataP;
            modemCmdWriteData_t *cmdWriteP = &dataMsgSmP->cmdWrite;
            // Check if there is another daily log to send
            // If so, it returns a non-zero value representing the length
            uint16_t length = storageMgr_getNextDailyLogToTransmit(&dataP, msgData.weeklyLogNum);
            if (length) {
                // Update the data message state machine so that it will be
                // in the correct state to send the next daily log.
                dataMsgSm_sendAnotherDataMsg(dataMsgSmP);
                // Update the data command object with info about the current
                // daily log to send
                cmdWriteP->cmd             = M_COMMAND_SEND_DATA;
                cmdWriteP->payloadMsgId    = MSG_TYPE_DAILY;  /* the payload type */
                cmdWriteP->payloadP        = dataP;   /* the payload pointer */
                cmdWriteP->payloadLength   = length;  /* size of the payload in bytes */
            } else {
                msgData.sendDailyLogs = false;
            }
        }

        // Call the data message state machine to perform work
        dataMsgSm_stateMachine(dataMsgSmP);

        // Check if the data message state machine is done with the
        // complete session
        if (dataMsgSmP->allDone) {
            msgData.sendDataMsgActive = false;
            if (dataMsgSmP->connectTimeout) {
                // Error case
                // Check if this already was a retry
                // If not, then schedule a retry.  If it was, then abort.
                if (msgData.retryCount < DATA_MSG_MAX_RETRIES) {
                    msgData.retryCount++;
                    msgData.sendDataMsgScheduled = true;
                    msgData.secsTillTransmit = DATA_MSG_DELAY_IN_SECONDS_TILL_RETX;
                }
            }
        }
    } else if (msgData.sendDataMsgScheduled) {
        if (msgData.secsTillTransmit > 0) {
            msgData.secsTillTransmit--;
        }
        if (msgData.secsTillTransmit == 0) {
            // The sendWaterMsg function will clear the retryCount.
            // We need to save the current value so we can restore it.
            uint8_t retryCount = msgData.retryCount;
            // Use the standard data msg API to initiate the retry.
            // For retries, we assume the data is already stored in the modem
            // from the original try. We only have to "kick" the modem with any
            // type of M_COMMAND_SEND_DATA msg to get it to send out what it has
            // stored in its FIFOs (once its connected to the network).
            dataMsgMgr_sendDataMsg(MSG_TYPE_RETRYBYTE, NULL, 0);
            // Restore retryCount value.
            msgData.retryCount = retryCount;
        }
    }
}

/**
* \brief Send an outpour data msg to the modem. This kicks off a
*        multi-module/multi state machine sequence for sending
*        the data command to the modem.
* \ingroup PUBLIC_API
*  
* \note If the modem does not connect to the network within a
*       specified time frame (WAIT_FOR_LINK_UP_TIME_IN_SECONDS),
*       then one retry will be scheduled for the future.
* 
* @param msgId The outpour message identifier
* @param dataP Pointer to the data to send
* @param lengthInBytes The length of the data to send.
*/
bool dataMsgMgr_sendDataMsg(MessageType_t msgId, uint8_t *dataP, uint16_t lengthInBytes) {
    dataMsgSm_t *dataMsgSmP = &msgData.dataMsgSm;

    // If already busy, just return.
    // Should never happen, but just in case.
    if (msgData.sendDataMsgActive || msgData.sendFaActive) {
        return false;
    }

    // Note - a new data transmission will cancel any scheduled re-transmission

    msgData.sendDataMsgActive = true;
    msgData.sendDataMsgScheduled = false;
    msgData.sendFaScheduled = false;
    msgData.retryCount = 0;
    msgData.secsTillTransmit = 0;

    // Initialize the data msg object.
    dataMsgSm_initForNewSession(dataMsgSmP);

    // Initialize the data command object in the data object
    dataMsgSmP->cmdWrite.cmd             = M_COMMAND_SEND_DATA;
    dataMsgSmP->cmdWrite.payloadMsgId    = msgId;          /* the payload type */
    dataMsgSmP->cmdWrite.payloadP        = dataP;          /* the payload pointer */
    dataMsgSmP->cmdWrite.payloadLength   = lengthInBytes;  /* size of the payload in bytes */

    // Call the state machine
    dataMsgSm_stateMachine(dataMsgSmP);

    return true;
}

/**
 * 
 * \brief Inform the data message manager to start sending out 
 *        the daily logs of the current week.  This function
 *        will start the first daily log that needs to be sent.
 *        The function queries the storage module to get the
 *        next daily log to send.  Then initializes the command
 *        write structure and calls the data message state
 *        machine to start the transmit sequence.  The calls to
 *        get the remaining data logs for the week to send is
 *        done by the data message manager exec function.
* \ingroup PUBLIC_API
 * 
 * \param weeklyLogNum Which weekly log that the daily logs to
 *        send live in.
 * 
 * \return bool Returns false if the data message manager is 
 *         already busy transmitting and currently not
 *         available.
 */
bool dataMsgMgr_sendDailyLogs(uint8_t weeklyLogNum) {
    uint8_t *dataP;
    uint16_t length = 0;

    // If already busy, just return.
    // Should never happen, but just in case.
    if (msgData.sendDataMsgActive || msgData.sendFaActive) {
        return false;
    }

    // Get the first daily log to send.
    length = storageMgr_getNextDailyLogToTransmit(&dataP, weeklyLogNum);
    if (length) {
        dataMsgSm_t *dataMsgSmP = &msgData.dataMsgSm;
        modemCmdWriteData_t *cmdWriteP = &dataMsgSmP->cmdWrite;

        // Note - a new data transmission will cancel any scheduled re-transmission

        msgData.sendDataMsgActive = true;
        msgData.sendDailyLogs = true;
        msgData.sendDataMsgScheduled = false;
        msgData.sendFaScheduled = false;
        msgData.retryCount = 0;
        msgData.secsTillTransmit = 0;
        msgData.weeklyLogNum = weeklyLogNum;

        // Initialize the data message object so its ready to
        // start processing a new message.
        dataMsgSm_initForNewSession(dataMsgSmP);

        // Initialize the data command object used to communicate payload
        // pointer and length to the data message manager.
        cmdWriteP->cmd           = M_COMMAND_SEND_DATA;
        cmdWriteP->payloadMsgId  = MSG_TYPE_DAILY;  /* the payload type */
        cmdWriteP->payloadP      = dataP;   /* the payload pointer */
        cmdWriteP->payloadLength = length;  /* size of the payload in bytes */

        // Call the data message state machine to perform work
        dataMsgSm_stateMachine(dataMsgSmP);
    }
    return true;
}

/*******************************************************************************/
/*******************************************************************************/

/***************************
 * Module Data Definitions
 **************************/

/**
 * \def FASS_MSG_NUM_TO_SEND
 * \brief Specify how many times to send the final assembly msg 
 */
#define FASS_MSG_NUM_TO_SEND ((uint8_t)2)

/**
 * \def FASS_MSG_DELAY_IN_SEC_BETWEEN_SENDS
 * \brief Specify how long to wait between sending the FA 
 *        messages.
 */
#define FASS_MSG_DELAY_IN_SEC_BETWEEN_SENDS ((sys_tick_t)60)

/****************************
 * Module Data Declarations
 ***************************/

/*************************
 * Module Prototypes
 ************************/

/***************************
 * Module Public Functions
 **************************/

/**
* \brief Call once as system startup to initialize the final 
*        assembly module.
* \ingroup PUBLIC_API
*/
void fassMsgMgr_init(void) {
    // Currently does nothing
}

/**
* \brief Exec routine should be called once every time from the 
*        main processing loop.  It drives the  data message
*        state machine if there is a final assembly message
*        transmit currently in progress.
* \ingroup EXEC_ROUTINE
*/
void fassMsgMgr_exec(void) {

    // The final assembly message is only used once after boot up.
    // So it should never interfere with sending another data message type.
    // But just in case do a check.
    if (msgData.sendDataMsgActive || msgData.sendDataMsgScheduled) {
        return;
    }

    // If we are not active but scheduled....
    if (!msgData.sendFaActive && msgData.sendFaScheduled) {

        // Update the time till we send the packet
        if (msgData.secsTillTransmit > 0) {
            msgData.secsTillTransmit--;
        }
        // If the time has come to send the packet...prepare and send it
        if (msgData.secsTillTransmit == 0) {
            dataMsgSm_t *dataMsgSmP = &msgData.dataMsgSm;
            uint8_t *payloadP;
            uint8_t payloadSize;

            // We are officially active
            msgData.sendFaActive = true;

            // Prepare the message
            // Get the shared buffer (we borrow the ota buffer)
            payloadP = modemMgr_getSharedBuffer();
            // Fill in the buffer with the standard message header
            payloadSize = storageMgr_prepareMsgHeader(payloadP);

            // Initialize the data msg object that is used to communicate
            // with the data message state machine.
            dataMsgSm_initForNewSession(dataMsgSmP);

            // Initialize the data command object in the data object
            dataMsgSmP->cmdWrite.cmd             = M_COMMAND_SEND_DATA;
            // Only send a final assembly for the first message
            // Otherwise send a checkin message
            if (msgData.retryCount > 0 ) {
                dataMsgSmP->cmdWrite.payloadMsgId    = MSG_TYPE_CHECKIN;   /* the payload type */
            } else {
                dataMsgSmP->cmdWrite.payloadMsgId    = MSG_TYPE_FA;        /* the payload type */
            }
            dataMsgSmP->cmdWrite.payloadP        = payloadP;      /* the payload pointer */
            dataMsgSmP->cmdWrite.payloadLength   = payloadSize;   /* size of the payload in bytes */
        }
    }

    // If currently sending - drive the data message state machine
    // and check for done.
    if (msgData.sendFaActive) {
        // Get the pointer to the dataMsg object
        dataMsgSm_t *dataMsgSmP = &msgData.dataMsgSm;
        // Run the dataMsg state machine
        dataMsgSm_stateMachine(dataMsgSmP);
        // Check if state machine has completed sending
        if (dataMsgSmP->allDone) {
            msgData.sendFaActive = false;
            msgData.retryCount++;
            // Check if we should send again.  The FA message is repeated.
            if (msgData.retryCount < FASS_MSG_NUM_TO_SEND) {
                msgData.sendFaScheduled = true;
                msgData.secsTillTransmit = FASS_MSG_DELAY_IN_SEC_BETWEEN_SENDS;
            } else {
                msgData.sendFaScheduled = false;
            }

            // Update the Application Record after sending the first FA packet.
            // The record is written by the Application and used by the
            // Bootloader to help identify that the Application started successfully.
            // We wait until after the first FA packet is sent to write the
            // Application Record in an attmpt to help verify that the
            // application code does not have a catastrophic issue.
            // If the bootloader does not see an Application
            // Record when it boots, then it will go into revovery mode.
            //
            // Don't write a new Application Record if one already exists.  It is
            // erased by the bootloader after a new firmware upgrade has been
            // performed before jumping to the new Application code.
            // The Application Record is located in the flash INFO C section.
            if (!checkForApplicationRecord()) {
                // If the record is not found, write one.
                initApplicationRecord();
            }
        }
    }
}

/**
* \brief Send an outpour final assembly msg to the modem. This 
*        kicks off a multi-module/multi state machine sequence
*        for sending the data command to the modem.
* \ingroup PUBLIC_API
*/
void fassMsgMgr_sendFassMsg(void) {
    // The final assembly message is only used once after boot up.
    // So it should never interfere with sending another data message type.
    // But just in case do a check.
    if (msgData.sendDataMsgActive || msgData.sendDataMsgScheduled) {
        return;
    }
    msgData.sendFaScheduled = true;
    msgData.sendFaActive = false;
    msgData.retryCount = 0;
    msgData.secsTillTransmit = FASS_MSG_DELAY_IN_SEC_BETWEEN_SENDS;
}

