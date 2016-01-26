/** 
 * @file storage.c
 * \n Source File
 * \n Outpour MSP430 Firmware
 * 
* \brief Routines to handle/support storing water data and 
 *        managing red flag conditions.
 */

#include "outpour.h"

/***************************
 * Module Data Definitions
 **************************/

/**
 * \def SEND_DAILY_LOG
 * \brief The is used to control whether the dailyLog should be
 *        sent every day or not.  If set to 1, then the dailyLog
 *        will be sent after the end of each storage day.  If
 *        set to 0, then the daily logs will be sent after the
 *        end of the storage week.
 */
#define SEND_DAILY_LOG 1

/**
 * \def WEEKLY_LOG_NUM_MAX
 * \brief Specify the number of weekly logs in flash.
 */
#define WEEKLY_LOG_NUM_MAX ((uint8_t)2)

/**
 * \def WEEKLY_LOG_SIZE
 * \brief Specify the total size of a weekly block allocated in 
 *        flash.
 */
#define WEEKLY_LOG_SIZE ((uint16_t)0x400)

/**
 * \def TOTAL_DAYS_IN_A_WEEK
 * \brief For clarity in the code
 */
#define TOTAL_DAYS_IN_A_WEEK ((uint8_t)7)

/**
 * \def TOTAL_HOURS_IN_A_DAY
 * \brief For clarity in the code
 */
#define TOTAL_HOURS_IN_A_DAY ((uint8_t)24)

/**
 * \def TOTAL_SECONDS_IN_A_MINUTE
 * \brief For clarity in the code
 */
#define TOTAL_SECONDS_IN_A_MINUTE ((uint8_t)60)

/**
 * \def TOTAL_MINUTES_IN_A_HOUR
 * \brief For clarity in the code
 */
#define TOTAL_MINUTES_IN_A_HOUR ((uint8_t)60)

/**
 * \def FLASH_WRITE_ONE_BYTE
 * \brief For clarity in the code
 */
#define FLASH_WRITE_ONE_BYTE ((uint8_t)1)

/**
 * \def DAILY_LITERS_ACTIVATION_THRESHOLD
 * \brief Used as the daily liters threshold to consider the 
 *        unit activated.
 */
#define DAILY_LITERS_ACTIVATION_THRESHOLD ((uint16_t)200)

/**
 * \def MIN_DAILY_LITERS_TO_SET_REDFLAG_CONDITION
 * \brief The daily liters must meet this threshold before a red
 *        flag condition can be set.
 */
#define MIN_DAILY_LITERS_TO_SET_REDFLAG_CONDITION ((uint16_t)200)

/**
 * \def FLASH_BLOCK_SIZE
 * \brief Define how big a flash block is.  Represents the 
 *        minimum size that can be erased.
 */
#define FLASH_BLOCK_SIZE ((uint16_t)512)


/**
 * \typedef dailyHeader_t
 * \brief Define the structure of the header that sits on top of
 *        the daily log data in the daily packet.
 */
typedef struct __attribute__((__packed__))dailyHeader_s {
    uint8_t dummy0;                 /**< 0 */
    uint8_t dummy1;                 /**< 1 */
    uint8_t productId;              /**< 2 */
    uint8_t GMTsecond;              /**< 3 */
    uint8_t GMTminute;              /**< 4 */
    uint8_t GMThour;                /**< 5 */
    uint8_t GMTday;                 /**< 6 */
    uint8_t GMTmonth;               /**< 7 */
    uint8_t GMTyear;                /**< 8 */
    uint8_t fwMajor;                /**< 9 */
    uint8_t fwMinor;                /**< 10 */
    uint8_t daysActivatedMsb;       /**< 11 */
    uint8_t daysActivatedLsb;       /**< 12 */
    uint8_t weeks;                  /**< 13 */
    uint8_t reserve2;               /**< 14 */
    uint8_t reserve3;               /**< 15 */
} dailyHeader_t;

/**
 * \typedef dailyLog_t
 * \brief Define the structure of the daily log data that is 
 *        sent inside the daily packet.
 */
typedef struct __attribute__((__packed__))dailyLog_s {
    uint16_t liters[24];
    uint16_t padMax[6];
    uint16_t padMin[6];
    uint16_t padSubmerged[6];
    uint16_t comparedAverage;
    uint16_t unknowns;
    uint8_t redFlag;
} dailyLog_t;

typedef union packetHeader_s {
    dailyHeader_t dailyHeader;
    uint8_t bytes[16]; // force to 16 bytes
} packetHeader_t;

typedef union packetData_s {
    dailyLog_t dailyLog;
    uint8_t bytes[112]; // force to 112 bytes
} packetData_t;

/**
 * \typedef dailyPacket_t
 * \brief Define the structure of the daily log packet that is 
 *        sent to the server.  Note that it consists of a header
 *        and the data.  Both the header and the data sections
 *        are set to a specific size by using unions.  The total
 *        size of the packet is set at 126 bytes. Why?  because
 *        the message protocol adds two bytes at the top, making
 *        the data log message a nice even 128 bytes.
 */
typedef struct dailyPacket_s {
    packetHeader_t packetHeader;
    packetData_t packetData;
} dailyPacket_t;

/**
 * \typedef weeklyLog_t
 * \brief  Define the layout of the weekly log in flash.  It 
 *         currently consists of the 7 daily log packets and
 *         meta data.
 */
typedef struct weeklyLog_s {
    dailyPacket_t dailyPackets[7];       /**< The seven daily logs of the week */
    uint8_t clearOnTransmit[7];          /**< Byte cleared for day when log transmitted */
    uint8_t clearOnReady[7];             /**< Byte cleared for day when log ready to send */
} weeklyLog_t;

/**
 * \typedef storageData_t 
 * \brief Define a container to hold data for the storage 
 *        module.
 */
typedef struct storageData_s {
    uint16_t daysActivated;            /**< Total days activated */
    uint16_t currentMinuteML;          /**< Running sum for last minute */
    uint32_t currentHourML;            /**< Running sum for last hour */
    uint16_t dailyLiters;              /**< Running sum for last day */
    uint8_t storageTime_seconds;       /**< Current storage time - sec  */
    uint8_t storageTime_minutes;       /**< Current storage time - min */
    uint8_t storageTime_hours;         /**< Current storage time - hour */
    uint8_t storageTime_dayOfWeek;     /**< Current storage time - day */
    uint8_t storageTime_week;          /**< Current storage time - week */
    bool sendData;                     /**< True if ready to send data */
    bool alignStorageFlag;             /**< True if time to align storage time */
    uint8_t alignBcdSecond;            /**< Time to align at - sec */
    uint8_t alignBcdMinute;            /**< Time to align at - min */
    uint8_t alignBcdHour24;            /**< Time to align at - hour */
    sys_tick_t alignSafetyCheckInSec;  /**< Max time to wait for an align event */
    bool redFlagCondition;             /**< flag for red flag condition */
    uint8_t redFlagDayCount;           /**< running count of red flag days */
    uint8_t redFlagMapDay;             /**< used as index for red flag init mapping */
    bool redFlagDataFullyPopulated;    /**< true if redflag init mapping is completed */
    uint16_t redFlagThreshTable[7];    /**< store redFlag compare thresholds */
    uint8_t curWeeklyLogNum;           /**< Current weekly flash log working on */
    uint8_t prevWeeklyLogNum;          /**< Previous weekly flash log */
} storageData_t;

/****************************
 * Module Data Declarations
 ***************************/

// Allocate a section in flash
#pragma DATA_SECTION(week1Log, ".week1Data")
const weeklyLog_t week1Log;

// Allocate a section in flash
#pragma DATA_SECTION(week2Log, ".week2Data")
const weeklyLog_t week2Log;

// Force this table to be located in the .text area
#pragma DATA_SECTION(weeklyLogAddrTable, ".text")
static const weeklyLog_t *weeklyLogAddrTable[] = {
    &week1Log,
    &week2Log,
};

/**
 * \var stData 
 * \brief Declare the module data container 
 */
// static
storageData_t stData;

/********************* 
 * Module Prototypes
 *********************/

static void handle_red_flag(void);
static bool doesAlignTimeMatch(void);
static void recordLastMinute(void);
static void recordLastHour(void);
static void recordLastDay(void);
static weeklyLog_t* getWeeklyLogAddr(uint8_t weeklyLogNum);
static dailyLog_t* getDailyLogAddr(uint8_t weeklyLogNum, uint8_t dayOfTheWeek);
static dailyHeader_t* getDailyHeaderAddr(uint8_t weeklyLogNum, uint8_t dayOfTheWeek);
static dailyPacket_t* getDailyPacketAddr(uint8_t weeklyLogNum, uint8_t dayOfTheWeek);
static uint8_t getNextWeeklyLogNum(uint8_t weeklyLogNum);
static void eraseWeeklyLog(uint8_t weeklyLogNum);
static void prepareNextWeeklyLog(void);
static void prepareDailyLog(void);
static void markDailyLogAsReady(uint8_t dayOfTheWeek, uint8_t weeklyLogNum);
static bool isDailyLogReady(uint8_t dayOfTheWeek, uint8_t weeklyLogNum);
static void markDailyLogAsTransmitted(uint8_t dayOfTheWeek, uint8_t weeklyLogNum);
static bool wasDailyLogTransmitted(uint8_t dayOfTheWeek, uint8_t weeklyLogNum);
static void sendMonthlyCheckin(void);
// static void fillDailyLogWithRamp(uint8_t weeklyLogNum);
// uint16_t getSimulatedDailyLiters(uint8_t weekNum, uint8_t dayOfWeek);

/***************************
 * Module Public Functions
 **************************/

/**
* \brief Call once as system startup to initialize the storage 
*        module.
* \ingroup PUBLIC_API
*/
void storageMgr_init(void) {
    memset(&stData, 0, sizeof(storageData_t));
    storageMgr_resetWeeklyLogs();
    prepareDailyLog();
}

/**
* \brief This is the flash storage manager. It is called from 
*        the main processing loop.
* \ingroup EXEC_ROUTINE
*/
void storageMgr_exec(void) {

    // If we are waiting for an alignment event to occur, see if there
    // is a match (GMT time == alignment time).
    if (stData.alignStorageFlag == true) {
        // Decrement our safety down counter.
        // We use a safety down-counter to make sure that we don't wait more
        // than 24 hours for the alignment event to occur.
        if (stData.alignSafetyCheckInSec != 0) {
            stData.alignSafetyCheckInSec--;
        }
        if (doesAlignTimeMatch() || (stData.alignSafetyCheckInSec == 0)) {
            // If the current time is equal to the storage offset,
            // then zero storage time and clear storage memory
            stData.alignStorageFlag = false;
            stData.storageTime_seconds = 0;
            stData.storageTime_minutes = 0;
            stData.storageTime_hours = 0;
            stData.storageTime_dayOfWeek = 0;
            stData.storageTime_week = 0;
            storageMgr_resetWeeklyLogs();
            storageMgr_resetRedFlagAndMap();
        } else {
            // Don't start storing any data until we are officially aligned.
            return;
        }
    }

    stData.currentMinuteML += waterSense_getLastMeasFlowRateInML();

    // Increment the unit of time, record the amount of water, reset the previous unit of time
    stData.storageTime_seconds++;
    if (stData.storageTime_seconds == TOTAL_SECONDS_IN_A_MINUTE) {
        // Record data
        recordLastMinute();
        // Update time
        stData.storageTime_minutes++;
        stData.storageTime_seconds = 0;
    }
    if (stData.storageTime_minutes == TOTAL_MINUTES_IN_A_HOUR) {
        // Record data
        recordLastHour();
        // Update time
        stData.storageTime_hours++;
        stData.storageTime_minutes = 0;
    }
    if (stData.storageTime_hours == TOTAL_HOURS_IN_A_DAY) {
        // Record Data
        recordLastDay();
        // Update Time
        stData.storageTime_dayOfWeek++;
        stData.storageTime_hours = 0;


        // Prepare data storage for next day
        if (stData.storageTime_dayOfWeek < TOTAL_DAYS_IN_A_WEEK) {
            prepareDailyLog();
            // Increment days activated only if its a non-zero value.
            if (stData.daysActivated) {
                stData.daysActivated++;
            }
        }
    }
    if (stData.storageTime_dayOfWeek == TOTAL_DAYS_IN_A_WEEK) {
        // Update Time
        stData.storageTime_dayOfWeek = 0;
        stData.storageTime_week++;
        // Set to true to send weekly transmission of dailyLogs
        stData.sendData = true;
        // Prepare data storage for next week and day
        prepareNextWeeklyLog();
        prepareDailyLog();
        // Increment days activated only if its a non-zero value.
        if (stData.daysActivated) {
            stData.daysActivated++;
        }
    }
}

/**
* \brief Save the GMT time values when a new storage day should 
*        start. These will be compared to current GMT time for a
*        match.
* \ingroup PUBLIC_API
* 
* @param bcdSecond BCD second value received from cloud
* @param bcdMinute BCD minute value received from cloud
* @param bcdHour24 BCD hour value received from cloud
*/
void storageMgr_setStorageAlignmentTime(uint8_t bcdSecond, uint8_t bcdMinute, uint8_t bcdHour24) {

    bool bcdIsValid = true;
    stData.alignBcdSecond = bcdSecond;
    stData.alignBcdMinute = bcdMinute;
    stData.alignBcdHour24 = bcdHour24;
    // Validate BCD for legal values.
    if (isBcdMinSecValValid(bcdSecond) == false) {
        bcdIsValid = false;
    } else if (isBcdMinSecValValid(bcdMinute) == false) {
        bcdIsValid = false;
    } else if (isBcdHour24Valid(bcdHour24) == false) {
        bcdIsValid = false;
    }
    if (bcdIsValid) {
        // Set the flag that we are waiting for an alignment event
        stData.alignStorageFlag = true;
        if (1) {
            // We use a safety down-counter to make sure that we don't wait more
            // than 24 hours for the alignment event to occur.  Calculate the
            // difference in hours, add an extra hour and convert to seconds.
            // This will be the max time window to allow the alignment event
            // to occur.
            volatile sys_tick_t timeDiffSeconds = 0;
            int8_t binNowHour = bcd_to_char(get24Hour());
            int8_t binAlignHour = bcd_to_char(bcdHour24);
            // Use signed result - can be negative
            int8_t timeDiffHours = binAlignHour - binNowHour;
            // If negative, add 24 hours
            if (timeDiffHours < 0) {
                timeDiffHours += TOTAL_HOURS_IN_A_DAY;
            }
            // Double check
            if ((timeDiffHours < 0) || (timeDiffHours > 23)) {
                timeDiffHours = 0;
            }
            // Add one additional hour to take into account max minutes and seconds (59m.59s)
            timeDiffHours += 1;
            // Convert to seconds
            timeDiffSeconds = timeDiffHours;
            timeDiffSeconds *= 60;  // convert to minutes
            timeDiffSeconds *= 60;  // convert to seconds
            stData.alignSafetyCheckInSec = ((sys_tick_t)timeDiffSeconds);
        }
    }
}

/**
* \brief Override the unit activation.  Either enable or 
*        disable.
* \ingroup PUBLIC_API
*
* @param flag set true or false 
*/
void storageMgr_overrideUnitActivation(bool flag) {
    stData.daysActivated = flag ? 1 : 0;
}

/**
* \brief Return the current unit activation status.
* \ingroup PUBLIC_API
* 
* @return bool true or false
*/
bool storageMgr_isUnitActivated(void) {
    return (stData.daysActivated ? true : false);
}

/**
* \brief Clear the redFlag in the records
* \ingroup PUBLIC_API
*/
void storageMgr_resetRedFlag(void) {
    stData.redFlagCondition = false;
}

/**
* \brief Clear the redFlag and redFlag map in the records
* \ingroup PUBLIC_API
*/
void storageMgr_resetRedFlagAndMap(void) {
    stData.redFlagCondition = false;
    stData.redFlagDataFullyPopulated = false;
    stData.redFlagMapDay = 0;
    stData.redFlagDayCount = 0;
    // Clear the thresh table containing the daily thresh
    memset(stData.redFlagThreshTable, 0, sizeof(stData.redFlagThreshTable));
}

/**
* \brief Resets flash for all weekly logs.  This erases all 
*        weekly log containers and resets the current weekly log
*        number.
* \ingroup PUBLIC_API
*/
void storageMgr_resetWeeklyLogs(void) {
    int i;
    stData.prevWeeklyLogNum = WEEKLY_LOG_NUM_MAX - 1;
    stData.curWeeklyLogNum = 0;
    for (i = 0; i < WEEKLY_LOG_NUM_MAX; i++) {
        eraseWeeklyLog(i);
    }
    return;
}

uint16_t reportLastMinute(void) {
    return stData.currentMinuteML;
}

uint32_t reportLastHour(void) {
    return stData.currentHourML;
}

/**
 * \brief This function is used to identify the next daily log of
 *        the current week that is ready to transmit.  If there
 *        is a log ready for transmit, the pointer and size are
 *        returned. If no daily log is ready for transmit, the
 *        function returns 0.  If a daily log pointer is
 *        returned, then that daily log is marked as being
 *        transmitted in the weekly info meta data.  The function
 *        starts searching the weekly info meta data for the first
 *        day of the week, and sequentially checks each day until
 *        a daily log is found that has not yet been transmitted.
* \ingroup PUBLIC_API
 * 
 * \param dataPP Pointer to a pointer that is filled in with the
 *               address of the daily log.
 * \param weeklyLogNum  Which weekly log to transmit from.
 * 
 * \return uint16_t Size of the daily log to send, otherwise set
 *         to zero if no daily log is ready to transmit
 */
uint16_t storageMgr_getNextDailyLogToTransmit(uint8_t **dataPP, uint8_t weeklyLogNum) {
    uint8_t i;
    uint16_t length = 0;
    // Loop for each day of the week until a daily log is found
    // that is ready and has not been transmitted.
    for (i = 0; i < TOTAL_DAYS_IN_A_WEEK; i++) {
        // Get the flags to check if daily log is ready
        // and has not yet been transmitted.
        bool logReady = isDailyLogReady(i, weeklyLogNum);
        bool wasTransmitted = wasDailyLogTransmitted(i, weeklyLogNum);
        if (logReady && !wasTransmitted) {
            // Get the address of the daily log
            dailyPacket_t *dpP = getDailyPacketAddr(weeklyLogNum, i);
            *dataPP = (uint8_t *)dpP;
            // The daily packet structure is defined so that it will be exactly 128
            // bytes in flash, and 128 bytes when we send it to the server.
            // However the first two bytes of the structure are redundant because
            // the modem command module adds these first two bytes for msgType and msgId.
            // Therefore, we need to skip these bytes when we send the packet
            // for transmission.  We do this by adjusting the pointer ahead by
            // two bytes, and also adjusting the size by two bytes (hence the "- 2").
            *dataPP += 2;
            length = sizeof(dailyPacket_t) - 2;
            // Mark this daily log as being transmitted
            markDailyLogAsTransmitted(i, weeklyLogNum);
            break;
        }
    }
    return length;
}

/**
* \brief Send debug information to the uart.  
* \ingroup PUBLIC_API
*/
void storageMgr_sendDebugDataToUart(void) {
    // Output debug information
    dbgMsgMgr_sendDebugMsg(MSG_TYPE_DEBUG_STORAGE_INFO, (uint8_t *)&stData, sizeof(storageData_t));
}

/*************************
 * Module Private Functions
 ************************/

static void sendMonthlyCheckin(void) {
    // Prepare the monthly check-in message
    // Get the shared buffer (we borrow the ota buffer)
    uint8_t *payloadP = modemMgr_getSharedBuffer();
    // Fill in the buffer with the standard message header
    uint8_t payloadSize = storageMgr_prepareMsgHeader(payloadP);
    // Initiate sending the monthly check-in message
    dataMsgMgr_sendDataMsg(MSG_TYPE_CHECKIN, payloadP, payloadSize);
}

/**
 * \brief Maintain the running sum for the hour.  At the end of 
 *        each minute, add currentMinuteML into the hourly
 *        running sum.
 */
static void recordLastMinute(void) {
    stData.currentHourML += stData.currentMinuteML;
    stData.currentMinuteML = 0;
    // At fifteen minutes, start sending the daily logs of the previous week
    // -OR- the monthly check-in message.
    if ((stData.sendData == true) && (stData.storageTime_minutes == 15)) {
        if (stData.daysActivated) {
            dataMsgMgr_sendDailyLogs(stData.prevWeeklyLogNum);
        } else if ((stData.storageTime_week % 4) == 0) {
            sendMonthlyCheckin();
        }
        stData.sendData = false;
    }
}

/**
 * \brief Write the total liters for the current hour into 
 *        flash. Update the running sum for the total daily
 *        liters.
 * \note The liters for the hour is stored in the current daily
 *       log contained in the current weekly log section.
 */
static void recordLastHour(void) {
    // Get pointer to today's dailyLog in flash.
    dailyLog_t *dailyLogsP = getDailyLogAddr(stData.curWeeklyLogNum, stData.storageTime_dayOfWeek);
    // Get address to liters parameter in the dailyLog
    uint8_t *addr = (uint8_t *)&(dailyLogsP->liters[stData.storageTime_hours]);
    uint16_t litersForThisHour = 0;

    // Stored liters is in fixed point 11.5
    // 11 bits of "liter' information, 1/32nd of sub-liter precision
    // Maximum flow per hour is ~1500 Liters, 11 bits gives us 2048 maximum
    litersForThisHour = (stData.currentHourML >> 5) & 0xffff;  // currentHour stored as long, need to shift down into 16 bits
    msp430Flash_write_int(addr, litersForThisHour);

    // For daily total, remove the .5 decimal and only store whole liters
    stData.dailyLiters += (litersForThisHour >> 5);

    stData.currentHourML = 0;

#if 0
    // FIX ME!!!
    // FOR DEBUG ONLY
    if ((stData.storageTime_hours % 4) == 0) {
        sendMonthlyCheckin();
    }
#endif

}

/**
* \brief Write the pad statistics to the daily log.
* \note If a new redFlag condition has occurred, then send the 
*       daily logs completed for this week.
*/
static void recordLastDay(void) {
    uint16_t i = 0;
    uint8_t *addr;
    uint16_t val16;
    bool prevRedFlag = stData.redFlagCondition;
    bool newRedFlagCondition = false;

    // Get pointer to today's dailyLog in flash.
    dailyLog_t *dailyLogsP = getDailyLogAddr(stData.curWeeklyLogNum, stData.storageTime_dayOfWeek);

    // Write per PAD stats to flash
    for (i = 0; i < 6; i++) {
        addr = 	(uint8_t *)&(dailyLogsP->padMax[i]);
        val16 = waterSense_getPadStatsMax((padId_t)i);
        msp430Flash_write_int(addr, val16);

        addr = 	(uint8_t *)&(dailyLogsP->padMin[i]);
        val16 = waterSense_padStatsMin((padId_t)i);
        msp430Flash_write_int(addr, val16);

        addr = 	(uint8_t *)&(dailyLogsP->padSubmerged[i]);
        val16 = waterSense_getPadStatsSubmerged((padId_t)i);
        msp430Flash_write_int(addr, val16);

    }
    // Write overall stats to flash
    addr = 	(uint8_t *)&(dailyLogsP->unknowns);
    val16 = waterSense_getPadStatsUnknowns();
    msp430Flash_write_int(addr, val16);

#if 0
    // FIX ME!!!!
    // FOR TEST ONLY
    stData.dailyLiters = getSimulatedDailyLiters(stData.storageTime_week, stData.storageTime_dayOfWeek);
#endif

    // Mark the current daily log as ready in the weekly log meta data.
    markDailyLogAsReady(stData.storageTime_dayOfWeek, stData.curWeeklyLogNum);

    // Process redFlag conditions
    handle_red_flag();

    // Write the redFlag condition to the daily log
    msp430Flash_write_bytes((uint8_t *)&(dailyLogsP->redFlag), (uint8_t *)&stData.redFlagCondition, FLASH_WRITE_ONE_BYTE);

    // Write the red flag threshold value for today to the daily log
    msp430Flash_write_int((uint8_t *)&(dailyLogsP->comparedAverage), stData.redFlagThreshTable[stData.storageTime_dayOfWeek]);

    // Check if this is a new redFlag condition
    if ((prevRedFlag != stData.redFlagCondition) && (stData.redFlagCondition == true)) {
        newRedFlagCondition = true;
    }

    // If unit is activated, check if we should transmit information
    // If this is a new red flag event, then send all ready daily logs for this week.
    if (stData.daysActivated && newRedFlagCondition) {
        dataMsgMgr_sendDailyLogs(stData.curWeeklyLogNum);
    }

    // Determine if the unit should be activated.
    // If the number measured daily liters exceeds the threshold, then consider
    // the unit activated.  Unit is considered not-activated if daysActivated is 0.
    if ((!stData.daysActivated) && (stData.dailyLiters > DAILY_LITERS_ACTIVATION_THRESHOLD)) {
        stData.daysActivated++;
        // We reset the redFlag data when the unit becomes activated
        storageMgr_resetRedFlagAndMap();
    }

#if (SEND_DAILY_LOG==1)
    // If sending dailyLogs on a daily basis, then send it we are activated.
    if (stData.daysActivated) {
        dataMsgMgr_sendDailyLogs(stData.curWeeklyLogNum);
    }
#endif

    // Clear all the pad statistics
    waterSense_clearStats();

    // Reset the daily liter sum
    stData.dailyLiters = 0;
}

/**
* \brief Monitor for a redFlag condition.
*/
static void handle_red_flag(void) {
    //red flag populated?
    if (stData.redFlagDataFullyPopulated) {

        uint8_t dayOfTheWeek = stData.storageTime_dayOfWeek;
        uint16_t redFlagDayThreshValue = stData.redFlagThreshTable[dayOfTheWeek];

        if (stData.redFlagCondition) {
            // see if existing redFlag condition needs to be cleared
            uint32_t temp;
            temp = redFlagDayThreshValue + redFlagDayThreshValue + redFlagDayThreshValue;
            uint16_t threeFourths = (temp >> 2) & 0xffff;

            // If we are less than 91 days of red flag condition, increment red flag.
            // Once we hit 91, we don't need to increment anymore because all want
            // to know is that we are past 90 days.
            if (stData.redFlagDayCount < 91) {
                stData.redFlagDayCount += 1;
            }

            // If today's dailyLiters value is greater than 3/4 of the threshold value,
            // then clear the redFlag condition.
            if (stData.dailyLiters > threeFourths) {
                // Reset red flag
                storageMgr_resetRedFlag();
            }
            // If today's dailyLiters value is greater than 1/8 of the threshold value,
            // and we are beyond 90 days, then clear the redFlag condition and restart the redFlag mapping.
            else if ((stData.dailyLiters > (redFlagDayThreshValue >> 3)) && (stData.redFlagDayCount > 90)) {
                // Restart red flag mapping
                storageMgr_resetRedFlagAndMap();
            }

        } else {
            // Check that today's daily liters were at least 50% of threshold
            uint16_t halfExpected = redFlagDayThreshValue >> 1;
            if ((stData.dailyLiters < halfExpected) && (redFlagDayThreshValue > MIN_DAILY_LITERS_TO_SET_REDFLAG_CONDITION)) {
                // Red flag condition is met
                stData.redFlagCondition = true;
                stData.redFlagDayCount = 1;
            } else {
                // Update the thresh table with a new value based on 75% threshold and 25% today's dailyLiters
                uint32_t temp;
                temp = redFlagDayThreshValue + redFlagDayThreshValue + redFlagDayThreshValue + stData.dailyLiters;
                uint16_t newAverage = 0;
                newAverage = (temp >> 2) & 0xffff;
                stData.redFlagThreshTable[dayOfTheWeek] = newAverage;
            }
        }
    } else {
        // Put today's daily liters into todays thresh table entry - no averaging
        // We are trying to get a baseline of each days water usage for the first week.
        stData.redFlagThreshTable[stData.storageTime_dayOfWeek] = stData.dailyLiters;
        stData.redFlagMapDay++;
        if (stData.redFlagMapDay >= TOTAL_DAYS_IN_A_WEEK) {
            // Fully populated after one week.
            stData.redFlagDataFullyPopulated = true;
        }
    }
}

/**
* \brief Utility routine to check if the alignment time matches 
*        the current GMT time for seconds, minutes and hours.
* 
* @return bool Returns true if a match occurred.
*/
static bool doesAlignTimeMatch(void) {
    timePacket_t *bcdTimeP = getBcdTime();

    if ((bcdTimeP->second == stData.alignBcdSecond) &&
        (bcdTimeP->minute == stData.alignBcdMinute) &&
        (bcdTimeP->hour24 == stData.alignBcdHour24)) {
        return true;
    } else {
        return false;
    }
}

/**
*  \brief Utility function to get the address from the weekly
*         log number.
* 
* @param weeklyLogNum Weekly log number
* 
* @return weeklyLog_t*  Returns a pointer to the weekly log 
*         section.
*/
static weeklyLog_t* getWeeklyLogAddr(uint8_t weeklyLogNum) {
    const weeklyLog_t *wlP;
    if (weeklyLogNum < WEEKLY_LOG_NUM_MAX) {
        wlP = weeklyLogAddrTable[weeklyLogNum];
    } else {
        sysError();
    }
    return (weeklyLog_t *)wlP;
}

/**
* \brief Utility function to get the daily log address contained
*        in the weekly log.
* 
* @param weeklyLogNum  Which weekly log container
* @param dayOfTheWeek  Which day of the week.
* 
* @return dailyLog_t* Returns a pointer to the daily log
*/
static dailyLog_t* getDailyLogAddr(uint8_t weeklyLogNum, uint8_t dayOfTheWeek) {
    weeklyLog_t *wlP = getWeeklyLogAddr(weeklyLogNum);
    dailyLog_t *dailyLogP = &wlP->dailyPackets[dayOfTheWeek].packetData.dailyLog;
    return dailyLogP;
}

/**
* \brief   Utility function to get the address to the header 
*          portion of a daily packet.
* 
* @param weeklyLogNum Which weekly log container to access
* @param dayOfTheWeek Which day of the week
* 
* @return dailyHeader_t*  Pointer to the packet header.
*/
static dailyHeader_t* getDailyHeaderAddr(uint8_t weeklyLogNum, uint8_t dayOfTheWeek) {
    weeklyLog_t *wlP = getWeeklyLogAddr(weeklyLogNum);
    dailyHeader_t *dailyHeaderP = &wlP->dailyPackets[dayOfTheWeek].packetHeader.dailyHeader;
    return dailyHeaderP;
}

/**
* \brief Utility function to get the address to a daily packet.
* 
* @param weeklyLogNum Which weekly log container to access
* @param dayOfTheWeek Which day of the week
* 
* @return dailyHeader_t*  Pointer to the packet.
*/
static dailyPacket_t* getDailyPacketAddr(uint8_t weeklyLogNum, uint8_t dayOfTheWeek) {
    weeklyLog_t *wlP = getWeeklyLogAddr(weeklyLogNum);
    dailyPacket_t *dailyPacketP = &wlP->dailyPackets[dayOfTheWeek];
    return dailyPacketP;
}

/**
* \brief   Utility function to increment to the next weekly log.
*          Handles rollover condition.
* 
* @param weeklyLogNum Current weekly log number
* 
* @return uint8_t Next sequential weekly log number
*/
static uint8_t getNextWeeklyLogNum(uint8_t weeklyLogNum) {
    uint8_t nextWeeklyLogNum = weeklyLogNum + 1;
    if (nextWeeklyLogNum >= WEEKLY_LOG_NUM_MAX) {
        nextWeeklyLogNum = 0;
    }
    return nextWeeklyLogNum;
}

/**
* \brief Erase the weekly log container (in flash).
* 
* @param weeklyLogNum  The weekly log number.
*/
static void eraseWeeklyLog(uint8_t weeklyLogNum) {
    uint8_t *addr = (uint8_t *)getWeeklyLogAddr(weeklyLogNum);
    msp430Flash_erase_segment(addr);
    msp430Flash_erase_segment(addr + FLASH_BLOCK_SIZE);
}

/**
* \brief Advance to the next weekly log container (rollover if 
*        required).  Erase the identified weekly log container.
*/
static void prepareNextWeeklyLog(void) {
    volatile uint8_t curWeeklyLogNum = stData.curWeeklyLogNum;
    volatile uint8_t nextWeeklyLogNum = getNextWeeklyLogNum(curWeeklyLogNum);
    stData.prevWeeklyLogNum = curWeeklyLogNum;
    stData.curWeeklyLogNum = nextWeeklyLogNum;
    eraseWeeklyLog(nextWeeklyLogNum);
}

/**
* \brief Update the packet header portion of the daily log.
*/
static void prepareDailyLog(void) {
    dailyHeader_t *dailyHeaderP = getDailyHeaderAddr(stData.curWeeklyLogNum, stData.storageTime_dayOfWeek);
    timePacket_t *tp = getBinTime();
    uint8_t temp8;

    // Product ID
    temp8 = OUTPOUR_PRODUCT_ID;
    msp430Flash_write_bytes((uint8_t *)&(dailyHeaderP->productId), &temp8, FLASH_WRITE_ONE_BYTE);

    // Time
    msp430Flash_write_bytes((uint8_t *)&(dailyHeaderP->GMTsecond), &tp->second, FLASH_WRITE_ONE_BYTE);
    msp430Flash_write_bytes((uint8_t *)&(dailyHeaderP->GMTminute), &tp->minute, FLASH_WRITE_ONE_BYTE);
    msp430Flash_write_bytes((uint8_t *)&(dailyHeaderP->GMThour),   &tp->hour24, FLASH_WRITE_ONE_BYTE);
    msp430Flash_write_bytes((uint8_t *)&(dailyHeaderP->GMTday),    &tp->day,    FLASH_WRITE_ONE_BYTE);
    msp430Flash_write_bytes((uint8_t *)&(dailyHeaderP->GMTmonth),  &tp->month,  FLASH_WRITE_ONE_BYTE);
    msp430Flash_write_bytes((uint8_t *)&(dailyHeaderP->GMTyear),   &tp->year,   FLASH_WRITE_ONE_BYTE);

    // FW Version
    temp8 = FW_VERSION_MAJOR;
    msp430Flash_write_bytes((uint8_t *)&(dailyHeaderP->fwMajor), &temp8, FLASH_WRITE_ONE_BYTE);
    temp8 = FW_VERSION_MINOR;
    msp430Flash_write_bytes((uint8_t *)&(dailyHeaderP->fwMinor), &temp8, FLASH_WRITE_ONE_BYTE);

    // Days Activated
    msp430Flash_write_int((uint8_t *)&(dailyHeaderP->daysActivatedMsb), stData.daysActivated);

    // weeks
    temp8 = stData.storageTime_week;
    msp430Flash_write_bytes((uint8_t *)&(dailyHeaderP->weeks), &temp8, FLASH_WRITE_ONE_BYTE);

    // day of the week
    temp8 = stData.storageTime_dayOfWeek;
    msp430Flash_write_bytes((uint8_t *)&(dailyHeaderP->reserve2), &temp8, FLASH_WRITE_ONE_BYTE);

    temp8 = 0xA5;
    msp430Flash_write_bytes((uint8_t *)&(dailyHeaderP->reserve3), &temp8, FLASH_WRITE_ONE_BYTE);
}

/**
* \brief Updates the record for tracking that a daily log is
*        ready for transmit.
* 
* @param dayOfTheWeek The day of the week to record
* @param weeklyLogNum The weekly log container to use
*/
static void markDailyLogAsReady(uint8_t dayOfTheWeek, uint8_t weeklyLogNum) {
    uint8_t zeroVal = 0;
    weeklyLog_t *wlP = getWeeklyLogAddr(weeklyLogNum);
    if (dayOfTheWeek >= TOTAL_DAYS_IN_A_WEEK) {
        return;
    }
    uint8_t *entryP = &(wlP->clearOnReady[dayOfTheWeek]);
    msp430Flash_write_bytes(entryP, &zeroVal, FLASH_WRITE_ONE_BYTE);
}

/**
* \brief Utility function to check if a daily log is ready for
*        transmit.
* 
* @param dayOfTheWeek The day of the week to record
* @param weeklyLogNum The weekly log container to use
*/
static bool isDailyLogReady(uint8_t dayOfTheWeek, uint8_t weeklyLogNum) {
    weeklyLog_t *wlP = getWeeklyLogAddr(weeklyLogNum);
    bool isReady = !wlP->clearOnReady[dayOfTheWeek];
    return isReady;
}

/**
* \brief Updates the record for tracking that a daily log has 
*        been transmitted.
* 
* @param dayOfTheWeek The day of the week to record
* @param weeklyLogNum The weekly log container to use
*/
static void markDailyLogAsTransmitted(uint8_t dayOfTheWeek, uint8_t weeklyLogNum) {
    uint8_t zeroVal = 0;
    weeklyLog_t *wlP = getWeeklyLogAddr(weeklyLogNum);
    uint8_t *entryP = (uint8_t *)&(wlP->clearOnTransmit[dayOfTheWeek]);
    if (dayOfTheWeek >= TOTAL_DAYS_IN_A_WEEK) {
        return;
    }
    msp430Flash_write_bytes(entryP, &zeroVal, FLASH_WRITE_ONE_BYTE);
}

/**
* \brief Utility function to check if a daily log has been 
*        transmitted.
* 
* @param dayOfTheWeek The day of the week to record
* @param weeklyLogNum The weekly log container to use
*/
static bool wasDailyLogTransmitted(uint8_t dayOfTheWeek, uint8_t weeklyLogNum) {
    weeklyLog_t *wlP = getWeeklyLogAddr(weeklyLogNum);
    // If zero, it means we transmitted packet.
    return (wlP->clearOnTransmit[dayOfTheWeek] ? false : true);
}

/**
* \brief Update the packet header portion of the daily log.
*/
uint8_t storageMgr_prepareMsgHeader(uint8_t *dataPtr) {

    // The daily packet structure is defined so that it will be exactly 128
    // when we send it to the server. However the first two bytes are
    // redundant because the modem command module adds these first two bytes
    // for msgType and msgId. Therefore, we need to skip the first two bytes
    // of the dataHeader_t structure.  We do this by "adjusting" the pointer
    // that was passed in by two bytes, and also by subtracting two from the
    // return packet size (hence the "- 2").

    dailyHeader_t *dailyHeaderP = (dailyHeader_t *)(dataPtr - 2);
    timePacket_t *tp = getBinTime();
    // Product ID
    dailyHeaderP->productId = OUTPOUR_PRODUCT_ID;
    // Time
    dailyHeaderP->GMTsecond = tp->second;
    dailyHeaderP->GMTminute = tp->minute;
    dailyHeaderP->GMThour = tp->hour24;
    dailyHeaderP->GMTday = tp->day;
    dailyHeaderP->GMTmonth = tp->month;
    dailyHeaderP->GMTyear = tp->year;
    // FW Version
    dailyHeaderP->fwMajor = FW_VERSION_MAJOR;
    dailyHeaderP->fwMinor = FW_VERSION_MINOR;
    // Days Activated
    dailyHeaderP->daysActivatedMsb = stData.daysActivated >> 8;
    dailyHeaderP->daysActivatedLsb = stData.daysActivated & 0xFF;
    // weeks
    dailyHeaderP->weeks = stData.storageTime_week;

    // For Debug
    dailyHeaderP->reserve2 = IFG1;
    dailyHeaderP->reserve3 = 0xCC;

    return (sizeof(packetHeader_t) - 2);
}

#if 0
static void fillDailyLogWithRamp(uint8_t weeklyLogNum) {
    uint16_t i;
    uint16_t *addr;

    eraseWeeklyLog(weeklyLogNum);

    addr = (uint16_t *)getDailyPacketAddr(weeklyLogNum, 0);
    for (i = 0; i < 64; i++) {
        write_int_flash((uint8_t *)addr, i);
        addr++;
    }
    markDailyLogAsReady(0, weeklyLogNum);

    addr = (uint16_t *)getDailyPacketAddr(weeklyLogNum, 5);
    for (i = 0; i < 64; i++) {
        write_int_flash((uint8_t *)addr, i);
        addr++;
    }
    markDailyLogAsReady(5, weeklyLogNum);
}
#endif

/********************************************************************************/

#if 0
const int16_t testDayLiters[16][7] = {
    4096, 4096, 4096, 4096, 4096, 4096, 4096,   // WEEK 0
    4096, 4096, 4096, 4096, 4096, 4096, 4096,   // WEEK 1
    4096, 4096, 1024, 1024, 1024, 1024, 1024,   // WEEK 2

    1024, 1024, 1024, 1024, 1024, 1024, 1024,   // WEEK 3
    1024, 1024, 1024, 1024, 1024, 1024, 1024,   // WEEK 4
    1024, 1024, 1024, 1024, 1024, 1024, 1024,   // WEEK 5
    1024, 1024, 1024, 1024, 1024, 1024, 1024,   // WEEK 6

    1024, 1024, 1024, 1024, 1024, 1024, 1024,   // WEEK 7
    1024, 1024, 1024, 1024, 1024, 1024, 1024,   // WEEK 8
    1024, 1024, 1024, 1024, 1024, 1024, 1024,   // WEEK 9
    1024, 1024, 1024, 1024, 1024, 1024, 1024,   // WEEK 10

    1024, 1024, 1024, 1024, 1024, 1024, 1024,   // WEEK 11
    1024, 1024, 1024, 1024, 1024, 1024, 1024,   // WEEK 12
    1024, 1024, 1024, 1024, 1024, 1024, 1024,   // WEEK 13
    1024, 1024, 1024, 1024, 1024, 1024, 1024,   // WEEK 14

    1024, 1024, 1024, 1024, 1024, 1024, 1024,   // WEEK 15
};

const int16_t testDayLiters[8][7] = {
    1024, 1024, 1024, 1024, 1024, 1024, 1024,   // WEEK 0
    4096, 4096, 4096, 4096, 4096, 4096, 4096,   // WEEK 1
    4096, 4096, 4096, 4096, 4096, 4096, 4096,   // WEEK 2
    4096, 4096, 4096, 4096, 4096, 4096, 4096,   // WEEK 3
    4096, 4096, 4096, 4096, 4096, 4096, 4096,   // WEEK 4
    4096, 4096, 1024, 1024, 1024, 1024, 1024,   // WEEK 5
    1024, 1024, 1024, 1024, 1024, 1024, 1024,   // WEEK 6
    1024, 1024, 1024, 1024, 1024, 1024, 1024,   // WEEK 7
};

const int16_t testDayLiters[16][7] = {
    4096, 4096, 4096, 4096, 4096, 4096, 4096,   // WEEK 0
    4096, 4096, 4096, 4096, 4096, 4096, 4096,   // WEEK 1
    4096, 4096, 1024, 1024, 1024, 1024, 1024,   // WEEK 2

    1024, 1024, 1024, 1024, 1024, 1024, 1024,   // WEEK 3
    1024, 1024, 1024, 1024, 1024, 1024, 1024,   // WEEK 4
    4096, 4096, 4096, 4096, 4096, 4096, 4096,   // WEEK 5
    4096, 4096, 4096, 1024, 1024, 1024, 1024,   // WEEK 6

    1024, 1024, 1024, 1024, 1024, 1024, 1024,   // WEEK 7
    1024, 1024, 1024, 1024, 1024, 1024, 1024,   // WEEK 8
    4096, 4096, 4096, 4096, 4096, 4096, 4096,   // WEEK 9
    4096, 4096, 1024, 4096, 1024, 4096, 1024,   // WEEK 10

    1024, 1024, 1024, 1024, 1024, 1024, 1024,   // WEEK 11
    1024, 1024, 1024, 1024, 1024, 1024, 1024,   // WEEK 12
    4096, 4096, 4096, 4096, 4096, 4096, 4096,   // WEEK 13
    4096, 4096, 4096, 4096, 4096, 4096, 4096,   // WEEK 14

    1024, 1024, 1024, 1024, 1024, 1024, 1024,   // WEEK 15
};

uint16_t getSimulatedDailyLiters(uint8_t weekNum, uint8_t dayOfWeek) {
    uint8_t week = weekNum % 16;
    return testDayLiters[week][dayOfWeek];
}

#endif

