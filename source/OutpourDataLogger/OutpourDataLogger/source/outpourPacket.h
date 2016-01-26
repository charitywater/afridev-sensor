
/** 
*  @file dataLogger_exec.hpp
*
*/
#pragma once

#include <windows.h>
#include <stdint.h>
#include <time.h>
#include <stdio.h>
#include <string.h>
#include <conio.h>
#include <iostream>
#include <string>
#include <vector>
#include "dataLogger_timer.h"

typedef uint32_t sys_tick_t;

/**
 * \typedef MessageType_t
 * \brief Identify the different Outpour outgoing messages 
 *        identifiers.
 */
typedef enum messageType {
	MSG_TYPE_FA = 0x00,
	MSG_TYPE_DAILY = 0x01,
	MSG_TYPE_WEEKLY = 0x02,
	MSG_TYPE_OTAREPLY = 0x03,
	MSG_TYPE_RETRYBYTE = 0x04,
	MSG_TYPE_CHECKIN = 0x05,
    MSG_TYPE_DEBUG_PAD_STATS = 0x10,
    MSG_TYPE_DEBUG_STORAGE_INFO = 0x11,
    MSG_TYPE_DEBUG_UNIT_INFO = 0x12,
} MessageType_t;

/**
 * \typedef OtaOpcode_t 
 * \brief Identify each possible incomming Outpour OTA opcode.
 */
typedef enum otaOpcodes_e {
    OTA_OPCODE_GMT_CLOCKSET = 0x01,
    OTA_OPCODE_LOCAL_OFFSET = 0x02,
    OTA_OPCODE_RESET_ALL = 0x03,
    OTA_OPCODE_RESET_RED_FLAG = 0x04,
    OTA_OPCODE_ACTIVATE_DEVICE = 0x05,
    OTA_OPCODE_SILENCE_DEVICE = 0x06,
    OTA_OPCODE_UPDATE_CONSTANTS = 0x07,
    OTA_OPCODE_RESET_DEVICE = 0x08
} OtaOpcode_t;

/**
 * \typedef modem_command_t
 * \brief These are the SIM900 BodyTrace Command Messsage Types
 */
typedef enum modem_command_e {
    M_COMMAND_PING = 0x00,                    /**< PING */
    M_COMMAND_MODEM_INFO = 0x1,               /**< MODEM INFO */
    M_COMMAND_MODEM_STATUS = 0x2,             /**< MODEM STATUS */
    M_COMMAND_MESSAGE_STATUS = 0x3,           /**< MESSAGE STATUS */
    M_COMMAND_SEND_TEST = 0x20,               /**< SEND TEST */
    M_COMMAND_SEND_DATA = 0x40,               /**< SEND DATA */
    M_COMMAND_GET_INCOMING_PARTIAL = 0x42,    /**< GET INCOMING PARTIAL */
    M_COMMAND_DELETE_INCOMING = 0x43,         /**< DELETE INCOMING */
    M_COMMAND_SEND_DEBUG_DATA = 0x50,         /**< SEND DEBUG DATA - FOR OUTPUT INTERNAL DEBUG ONLY */
    M_COMMAND_POWER_OFF = 0xe0,               /**< POWER OFF */
} modem_command_t;

#define TOTAL_PADS 6

/**
 * \typedef unitInfo_t
 * \brief Define the structure of the header that sits on top of
 *        the daily log data in the daily packet.
 */
typedef struct unitInfo_s {
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
    uint8_t ifg1;                   /**< 14 */
    uint8_t reserve3;               /**< 15 */
} unitInfo_t;

typedef struct sensorStats_s {
    uint16_t lastMeasFlowRateInMl;       /**< Last flow rate calculated */
    uint8_t numOfSubmergedPads;          /**< Number of pads submerged for this meas */
    uint8_t submergedPadsBitMask;        /**< Which pads are submerged for this meas */
    uint16_t secondsOfNoWater;           /**< How many seconds of no water detected */
    uint16_t unknowns;                   /**< Overall count of unknown case seed (skipped pad) */
    int16_t tempCelcius;                 /**< Temperature reading from chip */
    uint16_t padCounts[TOTAL_PADS];      /**< holds raw cap reading for each pad */
    uint16_t pad_max[TOTAL_PADS];        /**< Per pad max measured cap meas seen */
    uint16_t pad_min[TOTAL_PADS];        /**< Per pad min measured cap meas seen */
    int16_t  padMeasDelta[TOTAL_PADS];   /**< holds diff between max and cap reading */
    uint16_t pad_submerged[TOTAL_PADS];  /**< Per pad count of submerged */
    uint16_t pad_max1_array[TOTAL_PADS]; /**< per-hour max compare array */
    uint16_t pad_max2_array[TOTAL_PADS]; /**< per-hour max compare array */
} sensorStats_t;

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
    bool weekReady;                    /**< True if ready to send week data */
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

