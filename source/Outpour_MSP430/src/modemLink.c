/** 
 * @file modemLink.c
 * \n Source File
 * \n Outpour MSP430 Firmware
 * 
 * \brief modem module responsible for bringing up the modem and 
 *        shutting it down.
 */

#include "outpour.h"

/***************************
 * Module Data Definitions
 **************************/

/**
 * \typedef modemPowerOnSeqState_t
 * \brief Define the different states to power on the modem.
 */
typedef enum modemPowerOnSeqState_e {
    MODEM_POWERUP_STATE_IDLE,
    MODEM_POWERUP_STATE_ALL_OFF,
    MODEM_POWERUP_STATE_DCDC,
    MODEM_POWERUP_STATE_LSVCC,
    MODEM_POWERUP_STATE_GSM_HIGH,
    MODEM_POWERUP_STATE_GSM_LOW,
    MODEM_POWERUP_STATE_INIT_WAIT,
    MODEM_POWERUP_STATE_READY,
} modemPowerOnSeqState_t;

/**
 * \typedef modemLinkData_t
 * \brief Module data structure.
 */
typedef struct modemLinkData_s {
    bool active;
    bool modemUp;
    sys_tick_t startTimestamp;
    modemPowerOnSeqState_t powerOnHwSeqState;
} modemLinkData_t;

/****************************
 * Module Data Declarations
 ***************************/

// static
modemLinkData_t mlData;

/********************* 
 * Module Prototypes
 *********************/

static bool modemPowerUpStateMachine(void);

/***************************
 * Module Public Functions
 **************************/

/**
* \brief Executive that manages sequencing the modem power up 
*        and power down.  Called on the 1 second tick rate to
*        handle bringing the modem up and down.
* \ingroup EXEC_ROUTINE
*/
void modemLink_exec(void) {
    if (mlData.active) {
        modemPowerUpStateMachine();
    }
}

/**
* \brief One time initialization for module.  Call one time 
*        after system starts or (optionally) wakes.
* \ingroup PUBLIC_API
*/
void modemLink_init(void) {
    memset(&mlData, 0, sizeof(modemLinkData_t));
}

/**
* \brief Start the modem power up sequence.  Will reset the 
*        state machine that manages the power up steps.
* \ingroup PUBLIC_API
*/
void modemLink_restart(void) {
    mlData.active = true;
    mlData.modemUp = false;
    mlData.powerOnHwSeqState = MODEM_POWERUP_STATE_ALL_OFF;
    mlData.startTimestamp = GET_SYSTEM_TICK();
    modemPowerUpStateMachine();
}

void modemLink_shutdownModem(void) {
    mlData.active = false;
    mlData.modemUp = false;
    mlData.powerOnHwSeqState = MODEM_POWERUP_STATE_IDLE;
    P1OUT &= ~GSM_DCDC;
    P1OUT &= ~LS_VCC;
}

bool modemLink_isModemUp(void) {
    return mlData.modemUp;
}

uint16_t modemLink_getModemUpTimeInSecs(void) {
    sys_tick_t onTime = GET_ELAPSED_TIME_IN_SEC(mlData.startTimestamp);
    uint16_t onTime16 = onTime;
    return onTime16;
}

/*************************
 * Module Private Functions
 ************************/

/**
* \brief State machine to sequence through the modem power on 
*        hardware steps. This is a timebased sequence to control
*        the hardware power on.
*/
static bool modemPowerUpStateMachine(void) {
    // Continue processing is used to determine whether
    // the do-while state processing loop should continue
    // for another iteration.
    bool continue_processing = false;
    volatile sys_tick_t onTime = GET_ELAPSED_TIME_IN_SEC(mlData.startTimestamp);
    switch (mlData.powerOnHwSeqState) {
    case MODEM_POWERUP_STATE_IDLE:
        break;
    case MODEM_POWERUP_STATE_ALL_OFF:
        P1OUT &= ~GSM_DCDC;
        P1OUT &= ~LS_VCC;
        if (onTime >= (1 * TIME_SCALER)) {
            mlData.powerOnHwSeqState = MODEM_POWERUP_STATE_DCDC;
        }
        break;
    case MODEM_POWERUP_STATE_DCDC:
        if (onTime >= (2 * TIME_SCALER)) {
            P1OUT |= GSM_DCDC;
            mlData.powerOnHwSeqState = MODEM_POWERUP_STATE_LSVCC;
        }
        break;
    case MODEM_POWERUP_STATE_LSVCC:
        if (onTime >= (3 * TIME_SCALER)) {
            P1OUT |= LS_VCC;
            mlData.powerOnHwSeqState = MODEM_POWERUP_STATE_GSM_HIGH;
        }
        break;
    case MODEM_POWERUP_STATE_GSM_HIGH:
        if (onTime >= (4 * TIME_SCALER)) {
            P1OUT |= GSM_EN;
            mlData.powerOnHwSeqState = MODEM_POWERUP_STATE_GSM_LOW;
        }
        break;
    case MODEM_POWERUP_STATE_GSM_LOW:
        if (onTime >= (10 * TIME_SCALER)) {
            P1OUT &= ~GSM_EN;
            mlData.powerOnHwSeqState = MODEM_POWERUP_STATE_INIT_WAIT;
        }
        break;
    case MODEM_POWERUP_STATE_INIT_WAIT:
        if (onTime >= (15 * TIME_SCALER)) {
            mlData.powerOnHwSeqState = MODEM_POWERUP_STATE_READY;
            mlData.modemUp = true;
        }
        break;
    case MODEM_POWERUP_STATE_READY:
        break;
    }
    return continue_processing;
}

