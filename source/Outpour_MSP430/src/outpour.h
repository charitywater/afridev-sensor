/** 
 * @file outpour.h
 * \n Source File
 * \n Outpour MSP430 Firmware
 * 
 * \brief MSP430 sleep control and support routines
 */

#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "msp430.h"
#include "msp430g2553.h"
#include "RTC_Calendar.h"
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
#define FW_VERSION_MAJOR ((uint8_t)0x02)

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
#define GET_SYSTEM_TICK() ((sys_tick_t)getSecondsSinceBoot());
// Return the number of elapsed seconds
#define GET_ELAPSED_TIME_IN_SEC(x) (((sys_tick_t)getSecondsSinceBoot())-(sys_tick_t)(x))

// Used for testing if running the sys tick at faster then
// the standard 1 second interval.  For normal operation, set to 1.
#define TIME_SCALER ((uint8_t)1)

/*******************************************************************************
* MISC Macros
*******************************************************************************/
/**
 * \def TIME_5_SECONDS 
 * \brief Macro for 5 seconds
 */
#define TIME_5_SECONDS ((uint8_t)5)
/**
 * \def TIME_10_SECONDS 
 * \brief Macro for 10 seconds
 */
#define TIME_10_SECONDS ((uint8_t)10)
/**
 * \def TIME_20_SECONDS 
 * \brief Macro for 20 seconds
 */
#define TIME_20_SECONDS ((uint8_t)20)
/**
 * \def TIME_30_SECONDS 
 * \brief Macro for 30 seconds
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
/**
 * \def TIME_5_MINUTES
 * \brief Macro to specify the number of seconds in 5 minutes
 */
#define TIME_5_MINUTES ((uint16_t)(SEC_PER_MINUTE*(uint16_t)5))
/**
 * \def TIME_10_MINUTES
 * \brief Macro to specify the number of seconds in 10 minutes
 */
#define TIME_10_MINUTES ((uint16_t)(SEC_PER_MINUTE*(uint16_t)10))
/**
 * \def TIME_20_MINUTES
 * \brief Macro to specify the number of seconds in 10 minutes
 */
#define TIME_20_MINUTES ((uint16_t)(SEC_PER_MINUTE*(uint16_t)20))

/**
 * \typedef padId_t
 * \brief Give each pad number a name
 */
typedef enum padId_e {
    PAD0,        /** 0 */
    PAD1,        /** 1 */
    PAD2,        /** 2 */
    PAD3,        /** 3 */
    PAD4,        /** 4 */
    PAD5,        /** 5 */
    TOTAL_PADS = 6,     /** there are 6 total pads */
    MAX_PAD = PAD5,     /** PAD5 is the max pad value */
} padId_t;

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
/**
 * \def REBOOT_KEY1
 * \def REBOOT_KEY2
 * \def REBOOT_KEY3
 * \def REBOOT_KEY4
 * \brief These keys are used to validate the OTA reset command.
 */
#define REBOOT_KEY1 ((uint8_t)0xAA)
#define REBOOT_KEY2 ((uint8_t)0x55)
#define REBOOT_KEY3 ((uint8_t)0xCC)
#define REBOOT_KEY4 ((uint8_t)0x33)

void sysExec_exec(void);
bool sysExec_startRebootCountdown(uint8_t *keysP);
void sysExec_doReboot(void);
void sysError(void);
void sysExec_sendDebugDataToUart(void);

/*******************************************************************************
* Utils.c
*******************************************************************************/
bool isBcdMinSecValValid(uint8_t bcdVal);
bool isBcdHour24Valid(uint8_t bcdVal);
unsigned int gen_crc16(const unsigned char *data, unsigned int size);
unsigned int gen_crc16_2buf(const unsigned char *data1, unsigned int size1, const unsigned char *data2, unsigned int size2);
void initApplicationRecord(void);
bool checkForApplicationRecord(void);

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
#define OTA_PAYLOAD_BUF_LENGTH ((uint8_t)48)

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
bool modemCmd_isResponseReady(void);
bool modemCmd_isError(void);
bool modemCmd_isBusy(void);

/*******************************************************************************
* modemLink.h
*******************************************************************************/
void modemLink_exec(void);
void modemLink_init(void);
void modemLink_restart(void);
void modemLink_shutdownModem(void);
bool modemLink_isModemUp(void);
uint16_t modemLink_getModemUpTimeInSecs(void);
bool modemLink_isModemUpError(void);

/*******************************************************************************
* modemMgr.c
*******************************************************************************/
void modemMgr_exec(void);
void modemMgr_init(void);
bool modemMgr_grab(void);
bool modemMgr_isModemUp(void);
bool modemMgr_isModemUpError(void);
void modemMgr_sendModemCmdBatch(modemCmdWriteData_t *cmdWriteP);
void modemMgr_stopModemCmdBatch(void);
bool modemMgr_isModemCmdComplete(void);
bool modemMgr_isModemCmdError(void);
void modemMgrRelease(void);
void modemMgr_restartModem(void);
bool modemMgr_isAllocated(void);
void modemMgr_release(void);
otaResponse_t* modemMgr_getLastOtaResponse(void);
bool modemMgr_isLinkUp(void);
bool modemMgr_isLinkUpError(void);
uint8_t modemMgr_getNumOtaMsgsPending(void);
uint8_t* modemMgr_getSharedBuffer(void);

/*******************************************************************************
* msgData.c
*******************************************************************************/
void dataMsgMgr_exec(void);
void dataMsgMgr_init(void);
bool dataMsgMgr_sendDataMsg(MessageType_t msgId, uint8_t *dataP, uint16_t lengthInBytes);
bool dataMsgMgr_sendDailyLogs(uint8_t weeklyLogNum);

/*******************************************************************************
* msgOta.c
*******************************************************************************/
void otaMsgMgr_exec(void);
void otaMsgMgr_init(void);
void otaMsgMgr_getAndProcessOtaMsgs(void);
void otaMsgMgr_stopOtaProcessing(void);
bool otaMsgMgr_isOtaProcessingDone(void);

/*******************************************************************************
* msgDebug.c
*******************************************************************************/

typedef struct eventSysInfo_s {
    uint8_t storageTime_seconds;       /**< Current storage time - sec  */
    uint8_t storageTime_minutes;       /**< Current storage time - min */
    uint8_t storageTime_hours;         /**< Current storage time - hour */
    uint8_t storageTime_dayOfWeek;     /**< Current storage time - day */
    uint8_t storageTime_week;          /**< Current storage time - week */
    uint16_t daysActivated;            /**< Total days activated */
    uint16_t currentMinuteML;          /**< Running sum for last minute */
    uint32_t currentHourML;            /**< Running sum for last hour */
    uint16_t dailyLiters;              /**< Running sum for last day */
} eventSysInfo_t;

typedef enum debugEvents_e {
    EVENT_RESET = 0x100,
    EVENT_SYS_INFO,
    EVENT_GMT_UPDATE,
    EVENT_CLOCK_ALIGNMENT,
    EVENT_ACTIVATION,
    EVENT_REDFLAG,
    EVENT_MODEM_TIMEOUT
} debugEvents_t;

void dbgMsgMgr_init(void);
void dbgMsgMgr_sendDebugMsg(MessageType_t msgId, uint8_t *dataP, uint16_t lengthInBytes);

/*******************************************************************************
* msgData.c
*******************************************************************************/
/**
 * \typedef dataMsgState_t
 * \brief Specify the states for sending a data msg to the 
 *        modem.
 */
typedef enum dataMsgState_e {
    DMSG_STATE_IDLE,
    DMSG_STATE_GRAB,
    DMSG_STATE_WAIT_FOR_MODEM_UP,
    DMSG_STATE_SEND_MSG,
    DMSG_STATE_SEND_MSG_WAIT,
    DMSG_STATE_WAIT_FOR_LINK,
    DMSG_STATE_PROCESS_OTA,
    DMSG_STATE_PROCESS_OTA_WAIT,
    DMSG_STATE_RELEASE,
} dataMsgState_t;

/**
 * \typedef dataMsgSm_t
 * \brief Define a contiainer to hold the information needed by 
 *        the data message module to perform sending a data
 *        command to the modem.
 *  
 * \note To save memory, this object can potentially be a common
 *       object that all clients use because only one client
 *       will be using the modem at a time?
 */
typedef struct dataMsgSm_s {
    dataMsgState_t dataMsgState;  /**< current data message state */
    modemCmdWriteData_t cmdWrite; /**< the command info object */
    uint8_t modemResetCount;      /**< for error recovery, count times modem is power cycled */
    bool sendCmdDone;             /**< flag to indicate sending the current command to modem is complete */
    bool allDone;                 /**< flag to indicate send session is complete and modem is off */
    bool connectTimeout;          /**< flag to indicate the modem was not able to connect to the network */
    bool commError;               /**< flag to indicate an modem UART comm error occured - not currently used - can remove */
} dataMsgSm_t;

void dataMsgSm_init(void);
void dataMsgSm_initForNewSession(dataMsgSm_t *dataMsgP);
void dataMsgSm_sendAnotherDataMsg(dataMsgSm_t *dataMsgP);
void dataMsgSm_stateMachine(dataMsgSm_t *dataMsgP);

/*******************************************************************************
* msgFinalAssembly.c
*******************************************************************************/
void fassMsgMgr_exec(void);
void fassMsgMgr_init(void);
void fassMsgMgr_sendFassMsg(void);

/*******************************************************************************
* time.c
*******************************************************************************/
/**
 * \typedef timePacket_t 
 * \brief Specify a structure to hold time data that will be 
 *        sent as part of the final assembly message.
 */
typedef struct  __attribute__((__packed__))timePacket_s {
    uint8_t second;
    uint8_t minute;
    uint8_t hour24;
    uint8_t day;
    uint8_t month;
    uint8_t year;
} timePacket_t;

void timerA1_init(void);
timePacket_t* getBinTime(void);
timePacket_t* getBcdTime(void);
uint8_t bcd_to_char(uint8_t bcdValue);
uint32_t getSecondsSinceBoot(void);
#if 0
void calibrateLoopDelay (void);
#endif

/*******************************************************************************
* storage.c
*******************************************************************************/
void storageMgr_init(void);
void storageMgr_exec(void);
void storageMgr_overrideUnitActivation(bool flag);
bool storageMgr_isUnitActivated(void);
void storageMgr_resetRedFlag(void);
void storageMgr_resetRedFlagAndMap(void);
void storageMgr_resetWeeklyLogs(void);
void storageMgr_setStorageAlignmentTime(uint8_t second, uint8_t minute, uint8_t hour);
void storageMgr_setDailyTransmission(bool enable);
void storageMgr_setWeeklyTransmission(bool enable);
uint16_t storageMgr_getNextDailyLogToTransmit(uint8_t **dataPP, uint8_t weeklyLogNum);
uint8_t storageMgr_prepareMsgHeader(uint8_t *dataPtr);
void storageMgr_sendDebugDataToUart(void);

/*******************************************************************************
* waterSense.c
*******************************************************************************/
void waterSense_init(void);
void waterSense_exec(void);
bool waterSense_isInstalled(void);
uint16_t waterSense_getLastMeasFlowRateInML(void);
void waterSense_clearStats(void);
uint16_t waterSense_getPadStatsMax(padId_t padId);
uint16_t waterSense_padStatsMin(padId_t padId);
uint16_t waterSense_getPadStatsSubmerged(padId_t padId);
uint16_t waterSense_getPadStatsUnknowns(void);
uint16_t padStats_zeros(void);
int16_t waterSense_getTempCelcius(void);
void waterSense_writeConstants(uint8_t *dataP);
void waterSense_sendDebugDataToUart(void);

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
void msp430Flash_write_int(uint8_t *flashP, uint16_t val16);
#if 0
void msp430flash_test(void);
#endif

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

#define APR_LOCATION ((uint8_t *)0x1040)  // INFO C
#define APR_MAGIC1 ((uint16_t)0x1234)
#define APR_MAGIC2 ((uint16_t)0x5678)

/**
 * \typedef appRecord_t
 * \brief This structure is used to put an application record in 
 *        one of the INFO sections.  The structue is used to
 *        tell the bootloader that application has started.
 */
typedef struct appRecord_e {
    uint16_t magic1;
    uint16_t magic2;
    uint16_t crc16;
} appRecord_t;

