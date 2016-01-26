/** 
 * @file msgDebug.c
 * \n Source File
 * \n Outpour MSP430 Firmware
 * 
 * \brief Manage the details of sending debug messages.
 */

#include "outpour.h"

/***************************
 * Module Data Definitions
 **************************/

/**
 * \typedef dbgState_t
 * \brief Specify the states for sending a debug msg to the 
 *        uart.
 */
typedef enum dbgState_e {
    DBG_STATE_IDLE,
    DBG_STATE_SEND_MSG,
    DBG_STATE_SEND_MSG_WAIT,
    DBG_STATE_DONE,
} dbgState_t;

/**
 * \typedef dbgData_t
 * \brief Define a container to store data specific to sending a 
 *        water msg to the modem.
 */
typedef struct dbgData_s {
    bool active;                  /**< marks the water msg manager as busy */
    dbgState_t dbgState;          /**< current state */
    modemCmdWriteData_t cmdWrite; /**< A pointer to the command info object */
} dbgData_t;

/****************************
 * Module Data Declarations
 ***************************/

/**
* \var dbgData
* \brief Declare a debug msg manager object.
*/
// static
dbgData_t dbgData;

/*************************
 * Module Prototypes
 ************************/
static void dbgMsgMgr_stateMachine(void);

/***************************
 * Module Public Functions
 **************************/

/**
* \brief Exec routine should be called once every time from the 
*        main processing loop.  Drives the debug message state
*        machine if there is a debug message transmit currently
*        in progress.  Debug messages are used to send data to
*        the data logger (an applicaiton running on a PC).
* \ingroup EXEC_ROUTINE
*/
void dbgMsgMgr_exec(void) {
    if (dbgData.active) {
        dbgMsgMgr_stateMachine();
    }
}

/**
* \brief Call once as system startup to initialize the water 
*        message manager object.
* \ingroup PUBLIC_API
*/
void dbgMsgMgr_init(void) {
    memset(&dbgData, 0, sizeof(dbgData_t));
}

/**
* \brief Send an outpour debug msg to the uart. 
* \ingroup PUBLIC_API
* 
* @param msgId The outpour message identifier
* @param dataP Pointer to the data to send
* @param lengthInBytes The length of the data to send.
*/
void dbgMsgMgr_sendDebugMsg(MessageType_t msgId, uint8_t *dataP, uint16_t lengthInBytes) {
    // Check if modem is busy.  If so, just exit
    if (modemMgr_isAllocated()) {
        return;
    }
    dbgData.active = true;
    dbgData.dbgState = DBG_STATE_SEND_MSG;
    memset(&dbgData.cmdWrite, 0, sizeof(modemCmdWriteData_t));
    dbgData.cmdWrite.cmd             = M_COMMAND_SEND_DEBUG_DATA;
    dbgData.cmdWrite.payloadMsgId    = msgId;          /* the payload type */
    dbgData.cmdWrite.payloadP        = dataP;          /* the payload pointer */
    dbgData.cmdWrite.payloadLength   = lengthInBytes;  /* size of the payload in bytes */
    dbgMsgMgr_stateMachine();
}

/*************************
 * Module Private Functions
 ************************/

static void dbgMsgMgr_stateMachine(void) {
    bool continue_processing = false;
    do {
        continue_processing = false;
        switch (dbgData.dbgState) {
        case DBG_STATE_IDLE:
            break;
        case DBG_STATE_SEND_MSG:
            modemCmd_write(&dbgData.cmdWrite);
            dbgData.dbgState = DBG_STATE_SEND_MSG_WAIT;
            modemCmd_exec();
            continue_processing = true;
            break;
        case DBG_STATE_SEND_MSG_WAIT:
            modemCmd_exec();
            if (!modemCmd_isBusy()) {
                dbgData.dbgState = DBG_STATE_DONE;
            }
            continue_processing = true;
            break;
        case DBG_STATE_DONE:
            dbgData.active = false;
            break;
        }
    } while (continue_processing);
}

