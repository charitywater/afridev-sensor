/** 
 * @file outpour.h
 * \n Source File
 * \n Outpour MSP430 Bootloader Firmware
 * 
 * \brief MSP430 sleep control and support routines
 */

#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "msp430.h"
#include "msp430g2553.h"
#include "modemMsg.h"

/**
 * \def OUTPOUR_PRODUCT_ID
 * \brief Specify the outpour product ID number that is sent in 
 *        messages.
 */
#define OUTPOUR_PRODUCT_ID ((uint8_t)0)

/**
 * \def FW_VERSION_MAJOR
 * \brief Specify the outpour firmware major version number.
 */
#define FW_VERSION_MAJOR ((uint8_t)0x01)

/**
 * \def FW_VERSION_MINOR
 * \brief Specify the outpour firmware minor version number.
 */
#define FW_VERSION_MINOR ((uint8_t)0x01)

/*******************************************************************************
* System Tick Access
*******************************************************************************/
// Define the type that a system tick value is represented in
typedef uint32_t sys_tick_t;
// Just return the system tick variable value.
#define GET_SYSTEM_TICK() ((sys_tick_t)getSysTicksSinceBoot());
// Return the number of elapsed system ticks since x
#define GET_ELAPSED_SYS_TICKS(x) (((sys_tick_t)getSysTicksSinceBoot())-(sys_tick_t)(x))

/**
 * \def SYS_TICKS_PER_SECOND
 * \brief Identify how many sys ticks occur per second. The main 
 *        exec loop runs at sys tick rate.
 */
#define SYS_TICKS_PER_SECOND ((uint16_t)32)

/*******************************************************************************
* MISC Macros
*******************************************************************************/
/**
 * \def TIME_30_SECONDS 
 * \brief Macro fo 30 seconds
 */
#define TIME_30_SECONDS ((uint8_t)30)
/**
 * \def TIME_60_SECONDS 
 * \brief Macro for 60 seconds
 */
#define TIME_60_SECONDS ((uint8_t)60)
/**
 * \def SECONDS_PER_MINUTE
 * \brief Macro to specify seconds per minute
 */
#define SECONDS_PER_MINUTE ((uint8_t)60)
/**
 * \def SECONDS_PER_HOUR
 * \brief Macro to specify seconds per hour
 */
#define SECONDS_PER_HOUR (SECONDS_PER_MINUTE*((uint16_t)60))
/**
 * \def TIME_ONE_HOUR
 * \brief Macro to specify one hour in terms of seconds
 */
#define TIME_ONE_HOUR SECONDS_PER_HOUR
/**
 * \def SEC_PER_MINUTE
 * \brief Macro to specify the number of seconds in one minute
 */
#define SEC_PER_MINUTE ((uint8_t)60)
/**
 * \def TIME_45_MINUTES
 * \brief Macro to specify the number of seconds in 45 minutes
 */
#define TIME_45_MINUTES ((uint16_t)(SEC_PER_MINUTE*(uint16_t)45))
/**
 * \def TIME_60_MINUTES
 * \brief Macro to specify the number of seconds in 60 minutes
 */
#define TIME_60_MINUTES ((uint16_t)(SEC_PER_MINUTE*(uint16_t)60))

#define LS_VCC BIT0
#define RXD BIT1
#define TXD BIT2
#define GSM_STATUS BIT4
#define GSM_INT BIT5
#define GSM_EN BIT6
#define GSM_DCDC BIT7

/*******************************************************************************
*  Centralized method for enabling and disabling MSP430 interrupts
*******************************************************************************/
static inline void enableGlobalInterrupt(void) {
    _BIS_SR(GIE);
}
static inline void disableGlobalInterrupt(void) {
    _BIC_SR(GIE);
}
static inline void enableSysTimerInterrupt(void) {
    TA1CCTL0 |= CCIE;
}
static inline void disableSysTimerInterrupt(void) {
    TA1CCTL0 &= ~CCIE;
}
static inline void restoreSysTimerInterrupt(uint16_t val) {
    TA1CCTL0 &= ~CCIE; // clear the value
    TA1CCTL0 |= val;   // set to val
}
static inline uint16_t getAndDisableSysTimerInterrupt(void) {
    volatile uint16_t current = TA1CCTL0; // read reg
    current  &= CCIE;  // get current interrupt setting
    TA1CCTL0 &= ~CCIE; // disable interrupt
    return current;    // return interrupt setting
}

/*******************************************************************************
*  Polling Delays
*******************************************************************************/
void secDelay(uint8_t secCount);
void ms1Delay(uint8_t msCount);
void us10Delay(uint8_t us10);

/*******************************************************************************
* sysExec.c
*******************************************************************************/
void sysExec_exec(void);
void sysExec_startRebootCountdown(uint8_t *keysP);
void sysExec_doReboot(void);
void sysError(void);

/*******************************************************************************
* Utils.c
*******************************************************************************/
unsigned int gen_crc16(const unsigned char *data, unsigned int size);

/*******************************************************************************
* modemCmd.h
*******************************************************************************/

/**
 * \typedef modemCmdWriteData_t 
 * \brief Container to pass parmaters to the modem command write 
 *        function.
 */
typedef struct modemCmdWriteData_s {
    modem_command_t cmd;         /**< the modem command */
    MessageType_t payloadMsgId;  /**< the payload type (Outpour message type) */
    uint8_t *payloadP;           /**< the payload pointer (if any) */
    uint16_t payloadLength;      /**< size of the payload in bytes */
    uint16_t payloadOffset;      /**< for receiving partial data */
    bool statusOnly;             /**< only perform status retrieve from modem - no cmd */
} modemCmdWriteData_t;

/**
 * \typedef modemCmdReadData_t 
 * \brief Container to read the raw response returned from the 
 *        modem as a result of sending it a command. 
 */
typedef struct modemCmdReadData_s {
    modem_command_t modemCmdId;    /**< the cmd we are sending to the modem */
    bool valid;                    /**< indicates that the response is correct (crc passed, etc) */
    uint8_t *dataP;                /**< the pointer to the raw buffer */
    uint8_t lengthInBytes;         /**< the length of the data in the buffer */
} modemCmdReadData_t;

/**
 * \def OTA_PAYLOAD_BUF_LENGTH
 * \brief Specify the size of our OTA buffer where the payload 
 *        portion of received OTA messages will be copied.
 */
#define OTA_PAYLOAD_BUF_LENGTH ((uint8_t)128)

/**
 * \typedef otaResponse_t
 * \brief Define a container to hold a partial OTA response.
 */
typedef struct otaResponse_s {
    uint8_t *buf;                         /**< A buffer to hold one OTA message */
    uint8_t lengthInBytes;                /**< how much valid data is in the buf */
    uint16_t remainingInBytes;            /**< how much remaining of the total OTA */
}otaResponse_t;

void modemCmd_exec(void);
void modemCmd_init(void);
bool modemCmd_write(modemCmdWriteData_t *writeCmdP);
void modemCmd_read(modemCmdReadData_t *readDataP);
bool modemCmd_isError(void);
bool modemCmd_isBusy(void);
void modemCmd_pollUart(void);

/*******************************************************************************
* modemLink.h
*******************************************************************************/
void modemLink_exec(void);
void modemLink_init(void);
void modemLink_restart(void);
void modemLink_shutdownModem(void);
bool modemLink_isModemUp(void);
bool modemLink_isModemUpError(void);
uint16_t modemLink_getModemUpTimeInSysTicks(void);

/*******************************************************************************
* modemMgr.c
*******************************************************************************/
void modemMgr_exec(void);
void modemMgr_init(void);
bool modemMgr_grab(void);
bool modemMgr_isModemUp(void);
void modemMgr_sendModemCmdBatch(modemCmdWriteData_t *cmdWriteP);
bool modemMgr_isModemCmdComplete(void);
bool modemMgr_isModemCmdError(void);
void modemMgrRelease(void);
void modemMgr_restartModem(void);
void modemMgr_release(void);
otaResponse_t* modemMgr_getLastOtaResponse(void);
bool modemMgr_isLinkUp(void);
bool modemMgr_isLinkUpError(void);
uint8_t modemMgr_getNumOtaMsgsPending(void);
uint16_t modemMgr_getSizeOfOtaMsgsPending(void);
uint8_t* modemMgr_getSharedBuffer(void);

/*******************************************************************************
* msgOta.c
*******************************************************************************/
/**
 * \typedef otaState_t
 * \brief Specify the different modes that the firmware update 
 *        state machine can exit in.
 */
typedef enum fwUpdateResult_e {
    RESULT_NO_FWUPGRADE_PERFORMED = 1,
    RESULT_GET_MORE_DATA = 2,
    RESULT_DONE_SUCCESS = 4,
    RESULT_DONE_ERROR = 8
} fwUpdateResult_t;

void otaMsgMgr_exec(void);
void otaMsgMgr_init(void);
void otaMsgMgr_getAndProcessOtaMsgs(bool sos);
bool otaMsgMgr_isOtaProcessingDone(void);
fwUpdateResult_t otaMsgMgr_getFwUpdateResult(void);

/*******************************************************************************
* time.c
*******************************************************************************/
void timerA0_0_init_for_sys_tick(void);
bool timerA0_0_check_for_sys_tick(void);
void timerA0_0_init_for_sleep_tick(void);
__interrupt void ISR_Timer0_A0(void);
uint32_t getSysTicksSinceBoot(void);
static inline void timerA0_0_halt(void) {
    TA0CCTL0 &= ~CCIFG;
    TA0CTL = 0;
}

// WDTPW+WDTCNTCL+WDTSSEL
// 1 second time out, uses ACLK
#define WATCHDOG_TICKLE() (WDTCTL = WDT_ARST_1000)
#define WATCHDOG_STOP() (WDTCTL = WDTPW | WDTHOLD)

/*******************************************************************************
* hal.c
*******************************************************************************/
void clock_init(void);
void uart_init(void);
void pin_init(void);

/*******************************************************************************
* flash.c
*******************************************************************************/
void msp430Flash_erase_segment(uint8_t *flashSectorAddrP);
void msp430Flash_write_bytes(uint8_t *flashP, uint8_t *srcP, uint16_t num_bytes);

/*******************************************************************************
* For Firmware Upgrade Support
*******************************************************************************/

/**
 * \def FLASH_UPGRADE_KEY1
 * \def FLASH_UPGRADE_KEY2
 * \def FLASH_UPGRADE_KEY3
 * \def FLASH_UPGRADE_KEY4
 * \brief These keys are used to validate the OTA firmware
 *        upgrade command.
 */
#define FLASH_UPGRADE_KEY1 ((uint8_t)0x31)
#define FLASH_UPGRADE_KEY2 ((uint8_t)0x41)
#define FLASH_UPGRADE_KEY3 ((uint8_t)0x59)
#define FLASH_UPGRADE_KEY4 ((uint8_t)0x26)

#define BLR_LOCATION ((uint8_t *)0x1080)  // INFO B
#define APR_LOCATION ((uint8_t *)0x1040)  // INFO C

#define BLR_MAGIC ((uint16_t)0x1234)

typedef struct bootloaderRecord_s {
    uint16_t magic;
    uint16_t bootRetryCount;
    uint16_t fwVersion;
    uint16_t crc16;
} bootloaderRecord_t;

#define APR_MAGIC1 ((uint16_t)0x1234)
#define APR_MAGIC2 ((uint16_t)0x5678)

typedef struct appRecord_e {
    uint16_t magic1;
    uint16_t magic2;
    uint16_t crc16;
} appRecord_t;

//
//   External variables from linker file
//
extern uint16_t _App_Reset_Vector;
extern uint16_t _App_Start;
extern uint16_t _App_End;
extern uint16_t _App_Length;
extern uint16_t __Boot_Start;
extern uint16_t _App_Proxy_Vector_Start[];

/*! Jumps to application using its reset vector address */
#define TI_MSPBoot_APPMGR_JUMPTOAPP()     {((void (*)()) _App_Reset_Vector) ();}
#define GET_APP_FLASH_START()             ((uint16_t)&_App_Start)
#define GET_APP_FLASH_END()               ((uint16_t)&_App_End)
#define GET_APP_VECTOR_TABLE()            ((uint16_t)_App_Proxy_Vector_Start)
#define GET_APP_RESET_VECTOR()            ((uint16_t)&_App_Reset_Vector)

#if 1
static inline uint16_t getAppFlashStartAddr(void) {
    return (uint16_t)&_App_Start;
}
static inline uint16_t getAppFlashEndAddr(void) {
    return (uint16_t)&_App_End;
}
static inline uint16_t getAppFlashLength(void) {
    return (uint16_t)_App_Length;
}
static inline uint16_t getAppVectorTableAddr(void) {
    return (uint16_t)_App_Proxy_Vector_Start;
}
static inline uint16_t getAppResetVector(void) {
    return (uint16_t)_App_Reset_Vector;
}
static inline uint16_t getBootFlashStartAddr(void) {
    return (uint16_t)&__Boot_Start;
}
#endif

