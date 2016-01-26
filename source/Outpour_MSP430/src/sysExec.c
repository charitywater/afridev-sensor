/** 
 * @file sysExec.c
 * \n Source File
 * \n Outpour MSP430 Firmware
 * 
 * \brief main system exec that runs the top level loop.
 */

#include "outpour.h"

/***************************
 * Module Data Definitions
 **************************/

/**
 * \def SEND_DEBUG_INFO_TO_UART
 * \brief If set to 1, debug data is sent to the UART every 
 *        iteration of the main loop (i.e. every one second).
 */
#define SEND_DEBUG_INFO_TO_UART 1

/**
 * \def REBOOT_DELAY_IN_SECONDS
 * \brief This define is used in conjunction with the OTA reset
 *        device message.  It specifies how long to wait after
 *        the message was received to perform the MSP430 reset.
 */
#define REBOOT_DELAY_IN_SECONDS ((uint8_t)45*TIME_SCALER)

/**
 * \typedef sysExecData_t
 * \brief Container to hold data for the sysExec module.
 */
typedef struct sysExecData_s {
    uint16_t secondsTillReboot;  /**< How long to wait to perform a MSP430 reset */
    uint8_t rebootKeys[4];       /**< The received keys with the reset OTA message */
    bool rebootActive;           /**< Specify if a reboot OTA message was received */
} sysExecData_t;

/****************************
 * Module Data Declarations
 ***************************/

// static
sysExecData_t sysExecData;

/*************************
 * Module Prototypes
 ************************/

// static void wdTimerTickle(void);

/***************************
 * Module Public Functions
 **************************/

/**
* \brief This is the main infinite software loop for the MSP430 
*        firmware.  After calling the initialization routines,
*        it drops into an infinite while loop.  In the while
*        loop, it calls the exec routines of the different
*        modules and then goes into a low power mode waiting for
*        the one second timer interrupt to bring it out of low
*        power mode.  
* 
* \ingroup EXEC_ROUTINE
*/
void sysExec_exec(void) {

    memset(&sysExecData, 0, sizeof(sysExecData_t));

    // Start the timer interrupt
    timerA1_init();

    // Initialize the date for Jan 1, 2015
    // h, m, s, am/pm
    setTime(0x00, 0x00, 0x00, 0x00);
    // y, m, d
    setDate(2015, 1, 1);

    // Call the module init routines
    modemLink_init();
    modemCmd_init();
    modemMgr_init();
    dataMsgSm_init();
    dataMsgMgr_init();
    otaMsgMgr_init();
    dbgMsgMgr_init();
    fassMsgMgr_init();
    waterSense_init();
    storageMgr_init();

    // Enable the global interrupt
    enableGlobalInterrupt();

    // The unit sends out the final assembly message after it boots
    fassMsgMgr_sendFassMsg();

    while (1) {

        // Water sensing and storage
        waterSense_exec();
        storageMgr_exec();

        // Communication
        modemCmd_exec();
        dataMsgMgr_exec();
        otaMsgMgr_exec();
        fassMsgMgr_exec();
        modemMgr_exec();
        modemCmd_exec();
        modemLink_exec();

#if (SEND_DEBUG_INFO_TO_UART==1)
        // Only send debug data if the modem is not in use.
        if (!modemMgr_isAllocated()) {
            sysExec_sendDebugDataToUart();
        }
#endif

        // sleep, wake on Timer1A interrupt
        __bis_SR_register(LPM3_bits);

        // If an OTA reset device message has been received, then rebootActive
        // will be true, waiting for the delay time to expire to perform
        // the MSP430 reboot.
        if (sysExecData.rebootActive) {
            if (sysExecData.secondsTillReboot > 0) {
                sysExecData.secondsTillReboot--;
            }
            if (sysExecData.secondsTillReboot == 0) {
                sysExec_doReboot();
            }
        }
    }
}

bool sysExec_startRebootCountdown(uint8_t *keysP) {
    bool status = false;
    if (keysP[0] == REBOOT_KEY1 &&
        keysP[1] == REBOOT_KEY2 &&
        keysP[2] == REBOOT_KEY3 &&
        keysP[3] == REBOOT_KEY4) {
        memcpy(sysExecData.rebootKeys, keysP, 4);
        sysExecData.secondsTillReboot = REBOOT_DELAY_IN_SECONDS;
        sysExecData.rebootActive = true;
        status = true;
    }
    return status;
}

void sysExec_doReboot(void) {
    if (sysExecData.rebootActive) {
        if (sysExecData.rebootKeys[0] == REBOOT_KEY1 &&
            sysExecData.rebootKeys[1] == REBOOT_KEY2 &&
            sysExecData.rebootKeys[2] == REBOOT_KEY3 &&
            sysExecData.rebootKeys[3] == REBOOT_KEY4) {
            while (1) {
                // Disable the global interrupt
                disableGlobalInterrupt();
                // Force watchdog reset
                WDTCTL = 0xDEAD;
                while (1);
            }
        } else {
            sysExecData.rebootActive = false;
        }
    }
}

#if (SEND_DEBUG_INFO_TO_UART==1)
/**
* \brief Send debug information to the uart.  
*/
void sysExec_sendDebugDataToUart(void) {
    // Get the shared buffer (we borrow the ota buffer)
    uint8_t *payloadP = modemMgr_getSharedBuffer();
    uint8_t payloadSize = storageMgr_prepareMsgHeader(payloadP);
    dbgMsgMgr_sendDebugMsg(MSG_TYPE_DEBUG_TIME_INFO, payloadP, payloadSize);
    _delay_cycles(10000);
    storageMgr_sendDebugDataToUart();
    _delay_cycles(10000);
    waterSense_sendDebugDataToUart();
    _delay_cycles(10000);
}
#endif

/***************************
 * Module Private Functions
 **************************/

#if 0
/**
* \brief Helper function to enable and tickle the watchdog.
*/
static void wdTimerTickle(void) {
    // WDTPW+WDTCNTCL+WDTSSEL
    // 1 second time out, uses ACLK
    WDTCTL = WDT_ARST_1000;
}
#endif

