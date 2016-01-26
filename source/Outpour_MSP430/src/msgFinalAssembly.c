/** 
 * @file msgFinalAssembly.c
 * \n Source File
 * \n Outpour MSP430 Firmware
 * 
 * \brief Manage the details of sending a Final Assembly 
 *        message.
 */

#include "outpour.h"

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
#define FASS_MSG_DELAY_IN_SEC_BETWEEN_SENDS ((sys_tick_t)180)

/**
 * \typedef fassData_t
 * \brief Define a container to store data specific to sending a
 *        final assembly msg to the modem.
 */
typedef struct fassData_s {
    bool active;                   /**< marks the final assembly msg manager as busy */
    bool sendScheduled;            /**< flag to mark a send is scheduled */
    uint8_t sendCount;             /**< number of FA's to send */
    sys_tick_t secsDelayTillSend;  /**< time in seconds until next send */
} fassData_t;

/****************************
 * Module Data Declarations
 ***************************/

/**
* \var fassData
* \brief Declare a final assemble msg manager object.
*/
// static
fassData_t fassData;

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
    memset(&fassData, 0, sizeof(fassData_t));
}

/**
* \brief Exec routine should be called once every time from the 
*        main processing loop.  Drives the  data message state
*        machine if there is a final assembly message transmit
*        currently in progress.
* \ingroup EXEC_ROUTINE
*/
void fassMsgMgr_exec(void) {

    // If we are not active but scheduled....
    if (!fassData.active && fassData.sendScheduled) {
        // Update the time till we send the packet
        if (fassData.secsDelayTillSend > 0) {
            fassData.secsDelayTillSend--;
        }
        // If the time has come to send the packet...prepare and send it
        if (fassData.secsDelayTillSend == 0) {
            dataMsgSm_t *dataMsgSmP;
            uint8_t *payloadP;
            uint8_t payloadSize;

            // We need to allocate the shared dataMsg object.  This is a shared memory
            // object used by multiple message clients.  There must only be one msg
            // client using it at one time.
            if (!dataMsgSm_isAllocated()) {
                dataMsgSm_allocate();
            } else {
                // Can't allocate, just return.
                return;
            }

            // Prepare the message
            // Get the shared buffer (we borrow part of the buffer used in the modemCmd module)
            payloadP = modemCmd_getSharedBuffer();
            // Fill in the buffer with the standard message header
            payloadSize = storageMgr_prepareMsgHeader(payloadP);

            // We are officially active
            fassData.active = true;
            // Initialize the data msg object that is used to communicate
            // with the data message state machine.
            dataMsgSmP = dataMsgSm_getObj();
            dataMsgSm_initForNewSession(dataMsgSmP);
            // Initialize the data command object in the data object
            dataMsgSmP->cmdWrite.cmd             = M_COMMAND_SEND_DATA;
            dataMsgSmP->cmdWrite.payloadMsgId    = MSG_TYPE_FA;   /* the payload type */
            dataMsgSmP->cmdWrite.payloadP        = payloadP;      /* the payload pointer */
            dataMsgSmP->cmdWrite.payloadLength   = payloadSize;   /* size of the payload in bytes */
        }
    }

    // If currently sending - drive the data message state machine
    // and check for done.
    if (fassData.active) {
        // Get the pointer to the dataMsg object
        dataMsgSm_t *dataMsgSmP = dataMsgSm_getObj();
        // Run the dataMsg state machine
        dataMsgSm_stateMachine(dataMsgSmP);
        // Check if state machine has completed sending
        if (dataMsgSmP->allDone) {
            fassData.active = false;
            // Release the shared dataMsg obj
            dataMsgSm_release();
            fassData.sendCount++;
            // Check if we should send again.  The FA message is repeated.
            if (fassData.sendCount < FASS_MSG_NUM_TO_SEND) {
                fassData.sendScheduled = true;
                fassData.secsDelayTillSend = FASS_MSG_DELAY_IN_SEC_BETWEEN_SENDS;
            } else {
                fassData.sendScheduled = false;
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
    // Note - a new transmission will cancel any previous scheduled
    fassData.sendScheduled = true;
    fassData.sendCount = 0;
    fassData.secsDelayTillSend = FASS_MSG_DELAY_IN_SEC_BETWEEN_SENDS;

}

