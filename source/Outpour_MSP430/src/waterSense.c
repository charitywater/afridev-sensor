/** 
 * @file waterSense.c
 * \n Source File
 * \n Outpour MSP430 Firmware
 * 
 * \brief Routines to support water sensing algorithm.
 */

#include "outpour.h"
#include "CTS_Layer.h"

/***************************
 * Module Data Definitions
 **************************/

/**
 * \def PERFORM_TEMPERATURE_MEAS
 * \brief Control if a temperature measurement should be taken 
 *        using the internal temp sensor (via ADC)
 */
#define PERFORM_TEMPERATURE_MEAS 1

/**
 * \def NO_WATER_HF_TO_LF_TIME_IN_SECONDS 
 * \brief  Specify how long to wait before switching to
 *         a 60 second measurement interval
 *         (move from high freq to Low freq rate).
 */
#define NO_WATER_HF_TO_LF_TIME_IN_SECONDS ((uint16_t)60*5)

/**
 * \typedef waterState_t
 * \brief Identify the states that the waterSense algorithm 
 *       flows through.
 */
typedef enum waterState_e {
    WATER_STATE_INSTALLED_LF,
    WATER_STATE_INSTALLED_HF,
} waterState_t;

/**
 * \typedef sensorStats_t
 * \brief define a type to hold stats from the water sensing 
 *        algorithm
 */
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

/****************************
 * Module Data Declarations
 ***************************/

/**
* \var int threshold[TOTAL_PADS]
* \brief Array used to hold thresholds used to identify if a pad 
*        is covered with water or not.  Used to compare against
*        the capacitive reading differences between max seen
*        (representing air) and current reading.
*/
const int16_t thresholdTable[TOTAL_PADS] = {
    489, // PAD 0 Threshold
    382, // PAD 1 Threshold
    600, // PAD 2 Threshold
    565, // PAD 3 Threshold
    386, // PAD 4 Threshold
    626  // PAD 5 Threshold
};

/**
* \var int highMarkFlowRates[7]
* \brief Array used to hold the milliliter per second flow rates 
*        values based on pad coverage.
*/
const uint16_t highMarkFlowRates[7] = {
    376, // Up through PAD 0 is covered with water
    335, // Up through PAD 1 is covered with water
    218, // Up through PAD 2 is covered with water
    173, // Up through PAD 3 is covered with water
    79,  // Up through PAD 4 is covered with water
    0,   // Only PAD 5 is covered with water -not used-ignore
    0    // No pads are covered
};


typedef struct wsData_s {
    bool prefillNextMaxTableFlag;      /**< flag to start saving cur max for next hour */
    uint16_t *pad_max_currentP;        /**< pointer to array that holds max values measured */
    uint16_t *pad_max_nextP;           /**< pointer to array that holds max values measured */
    uint32_t lastCalibrationTime;      /**< seconds since last calibration */
    waterState_t currentWaterState;    /**< variable to hold water sense state */
    waterState_t nextWaterState;       /**< variable to hold water sense state */
    uint8_t senseDelayUntilNextMeas;   /**< Delay in seconds until next measurement */
    sensorStats_t padStats;            /**< Array to hold stats */
    uint8_t initMaxArrayDelay;         /**< seconds to delay to init max array after boot */
    uint32_t lastTempMeasTime;         /**< seconds since last calibration */
    bool okToSwitchMaxTables;          /**< flag to indicated OK to switch current max table with next */
    uint16_t validateMaxTableCounter;  /**< no-water seconds counter to validate next max table */
} wsData_t;

/**
* \var wsData 
* \brief Module data structure 
*/
// static
wsData_t wsData;

/*************************
 * Module Prototypes
 ************************/

static uint16_t waterSense_takeReading(void);
static void doHighFreqReading(void);
static void doLowFreqReading(void);
static void check_calibration(void);
static void initMaxArray(void);
#if (PERFORM_TEMPERATURE_MEAS==1)
static int readInternalTemperature(void);
#endif

/***************************
 * Module Public Functions
 **************************/

/**
* \brief Call once as system startup to initialize the 
*        waterSense module.
* \ingroup PUBLIC_API
*/
void waterSense_init(void) {

    // zero module data structure
    memset(&wsData, 0, sizeof(wsData_t));

    // Init any nonzero values in the module data structure
    wsData.currentWaterState = WATER_STATE_INSTALLED_HF;
    wsData.nextWaterState = WATER_STATE_INSTALLED_HF;
    wsData.pad_max_currentP = &wsData.padStats.pad_max1_array[0];
    wsData.pad_max_nextP = &wsData.padStats.pad_max2_array[0];

    // Delay five seconds until first measurement
    wsData.senseDelayUntilNextMeas = 5;
    // Force pad max's to be initialized the first time the water meas is performed
    wsData.initMaxArrayDelay = 1;

    // Clear the stats
    waterSense_clearStats();

    // Perform initial pad readings to set max array
    initMaxArray();
}

/**
* \brief This is the default water sensing function. Identify 
*        the number of pads submerged, and run the water sense
*        state machine.
* \ingroup EXEC_ROUTINE
*/
void waterSense_exec(void) {

    // If we are in a low freq meas rate, we will delay between measurements.
    if (wsData.senseDelayUntilNextMeas > 0) {
        wsData.senseDelayUntilNextMeas--;
    }

    // Don't take measurements while the modem is transmitting.
    // It can change the measurement because of power draw on the system.
    else if (!modemMgr_isAllocated()) {

        // Perform the water measurement
        wsData.padStats.lastMeasFlowRateInMl = waterSense_takeReading();

        // Determine if we need to change the measurement rate.
        switch (wsData.currentWaterState) {
        default:
        case WATER_STATE_INSTALLED_LF:
            doLowFreqReading();
            break;
        case WATER_STATE_INSTALLED_HF:
            doHighFreqReading();
            break;
        }
        wsData.currentWaterState = wsData.nextWaterState;
    }

    // Determine if we should switch pad max/pad min tables
    check_calibration();

#if (PERFORM_TEMPERATURE_MEAS==1)
    // Take a temperature measurement every minute
    if ((wsData.lastTempMeasTime + TIME_60_SECONDS) <= getSecondsSinceBoot()) {
        wsData.padStats.tempCelcius = readInternalTemperature();
        wsData.lastTempMeasTime = getSecondsSinceBoot();
    }
#endif

    return;
}

/**
* \brief Write new constants to flash.  Writes pad thresholds 
*        and flow rate constants received from an OTA message to
*        the infoD section (0x1000).  Six 16 bit pad thresholds
*        and 7 16 bit flow rates.  Data arrives as MSB first.
* \ingroup PUBLIC_API
* 
* @param dataP A pointer to the constant received from the OTA 
*              message.
*/
void waterSense_writeConstants(uint8_t *dataP) {
    volatile uint8_t i;
    uint16_t val16;
    const uint16_t *dstP;

    // erase infoD segment
    msp430Flash_erase_segment((uint8_t *)&thresholdTable);

    // Write the pad thresholds
    dstP = (uint16_t *)thresholdTable;
    for (i = 0; i < 6; i++) {
        val16  = *dataP++;
        val16 |= *dataP++ << 8;
        msp430Flash_write_int((uint8_t *)dstP, val16);
        dstP++;
    }

    // Write the flow rates
    dstP = highMarkFlowRates;
    for (i = 0; i < 7; i++) {
        val16  = *dataP++;
        val16 |= *dataP++ << 8;
        msp430Flash_write_int((uint8_t *)dstP, val16);
        dstP++;
    }
}

/**
* \brief Send water sense debug information to the uart.  Dumps 
*        the complete sensorStats data structure.
* \ingroup PUBLIC_API
* 
*/
void waterSense_sendDebugDataToUart(void) {
    // Output debug information
    dbgMsgMgr_sendDebugMsg(MSG_TYPE_DEBUG_PAD_STATS, (uint8_t *)&wsData.padStats, sizeof(sensorStats_t));
}

/**
* \brief Return installed state: true or false
* \ingroup PUBLIC_API
* 
* @return uint8_t  true or false
*/
bool waterSense_isInstalled(void) {
    return ((wsData.currentWaterState == WATER_STATE_INSTALLED_LF) ||
            (wsData.currentWaterState == WATER_STATE_INSTALLED_HF));
}

/**
* \brief Return the last flow rate measured
* \ingroup PUBLIC_API
* 
* @return uint16_t  Flow rate in mL per second
*/
uint16_t waterSense_getLastMeasFlowRateInML(void) {
    return wsData.padStats.lastMeasFlowRateInMl;
}

/**
* \brief Return current max statistic
* \ingroup PUBLIC_API
*
* @return uint16_t Current max measured value for pad
*/
uint16_t waterSense_getPadStatsMax(padId_t padId) {
    return wsData.padStats.pad_max[padId];
}

/**
* \brief Return current min statistic
* \ingroup PUBLIC_API
*
* @return uint16_t Current min measured value for pad
*/
uint16_t waterSense_padStatsMin(padId_t padId) {
    return wsData.padStats.pad_min[padId];
}

/**
* \brief Return subMerged count statistic for pad
* \ingroup PUBLIC_API
*
* @return uint16_t Current submerged measured value for pad
*/
uint16_t waterSense_getPadStatsSubmerged(padId_t padId) {
    return wsData.padStats.pad_submerged[padId];
}

/**
* \brief Return Current unknown count statistic for pad
* \ingroup PUBLIC_API
*
* @return uint16_t Current unknowns count for pad
*/
uint16_t waterSense_getPadStatsUnknowns(void) {
    return wsData.padStats.unknowns;
}

/**
* \brief Return the temperature taken from the internal sensor.
* \ingroup PUBLIC_API
*
* @return int16_t Temperature in Celcius
*/
int16_t waterSense_getTempCelcius(void) {
    return wsData.padStats.tempCelcius;
}

/**
* \brief Clear saved statistics
*
* \ingroup PUBLIC_API
*/
void waterSense_clearStats(void) {
    wsData.padStats.tempCelcius = 0;
    wsData.padStats.unknowns = 0;
    volatile int i = 0;
    for (i = 0; i < TOTAL_PADS; i++) {
        wsData.padStats.pad_max[i] = 0;
        wsData.padStats.pad_min[i] = 0xffff;
        wsData.padStats.pad_submerged[i] = 0;
    }
}

/*************************
 * Module Private Functions
 ************************/

/**
* \brief Initialize the pad max arrays used to compare against 
*        current pad reading to determine if the pad is
*        submerged.  Only the current array is initialized.
* 
*/
static void initMaxArray(void) {
    volatile uint8_t padNum;
    for (padNum = 0; padNum < TOTAL_PADS; padNum++) {
        wsData.pad_max_currentP[padNum] = wsData.padStats.padCounts[padNum];
    }
}

/**
* 
* \brief Returns estimate of mL for this second. Will
*    update pad max values and next pad max values when
*    appropriate.
*
* \li INPUTS:  
* \li padCounts[TOTAL_PADS]: Taken from TI_CAPT_Raw. Generated
*     internally
* \li sense_next: Flag to start checking padCounts against
*     pad_max_next
* \li pad_max_current: Updated if necessary
*
* \li OUTPUTS:
* \li numOfSubmergedPads: Raw count of submerged sensors
* \li sensor_bits: 1 bit per sensor indicating submerged/not
* \li zeroes: count of samples with zero sensors submerged
* \li unknowns: count of samples with non-linear submerged
*     readings
* 
* @return uint16_t Estimate flow in milliLiters 
*/
static uint16_t waterSense_takeReading(void) {
    volatile uint16_t j;
    wsData.padStats.submergedPadsBitMask = 0;
    wsData.padStats.numOfSubmergedPads = 0;
    // Set the highestPad such that by default the flow rate lookup
    // value will be zero (in the flow rate table).
    uint8_t highestPad = TOTAL_PADS;
    bool waterNotSeenYet = true;

    // Perform the capacitive measurements
    TI_CAPT_Raw(&pad_sensors, &wsData.padStats.padCounts[0]);

    if (wsData.initMaxArrayDelay > 0) {
        wsData.initMaxArrayDelay--;
        if (wsData.initMaxArrayDelay == 0) {
            initMaxArray();
        }
    }

    // Loop for each pad.
    // Pad 5 is the lowest and will see water first.
    // Start with the top pad (0) and move towards bottom pad (5)
    for (j = 0; j < TOTAL_PADS; j++) {

        int16_t padMeasDelta = 0;
        uint16_t padMaxCompare = wsData.pad_max_currentP[j];
        uint16_t padCount = wsData.padStats.padCounts[j];
        int16_t thresholdVal = thresholdTable[j];

        // Rewriting comparison to avoid 'padAverage' and consider maximums instead
        // Get the difference between now and latest max value.
        // Higher values represent air.  Lower values represent water.
        // Determined by how many charge counts occur per second.
        // Takes longer for water to charge the capacitor detection circuitry.
        padMeasDelta = padMaxCompare - padCount;
        wsData.padStats.padMeasDelta[j] = padMeasDelta;

        // If the difference is greater than the calibrated threshold,
        // then assume pad is covered in water.
        if ((wsData.padStats.padMeasDelta[j] > 0) && (wsData.padStats.padMeasDelta[j] > thresholdVal)) {
            // Set binary bit representing pad to true.
            wsData.padStats.submergedPadsBitMask |= (1 << j);
            wsData.padStats.numOfSubmergedPads++;
            // Update statistics for pad, don't roll-over
            uint16_t padSubmergedCount = wsData.padStats.pad_submerged[j];
            if (padSubmergedCount != (uint16_t)0xFFFF) {
                wsData.padStats.pad_submerged[j]++;
            }
            // Mark the first pad from the top that sees water.
            // This will be used to figure out the current flow rate.
            if (waterNotSeenYet) {
                highestPad = j;
                waterNotSeenYet = false;
            }
        }

        // If this is a new max for the pad, record it
        if (wsData.padStats.padCounts[j] > wsData.pad_max_currentP[j]) {
            wsData.pad_max_currentP[j] = wsData.padStats.padCounts[j];
        }

        // If this is a new max and recording for next calibration set, record it
        if ((wsData.padStats.padCounts[j] > wsData.pad_max_nextP[j]) && (wsData.prefillNextMaxTableFlag)) {
            wsData.pad_max_nextP[j] = wsData.padStats.padCounts[j];
        }
        // If this is a new min, record it in tracking stats
        if (wsData.padStats.padCounts[j] < wsData.padStats.pad_min[j]) {
            wsData.padStats.pad_min[j] = wsData.padStats.padCounts[j];
        }
        // If this is a new max, record it in tracking stats
        if (wsData.padStats.padCounts[j] > wsData.padStats.pad_max[j]) {
            wsData.padStats.pad_max[j] = wsData.padStats.padCounts[j];
        }
    }

    // We are ignoring PAD5 based on the hardware issues with the PAD5
    // measurement.  Pad-5 is represented by the bit mask 0x20 (bit 6).
    // Clear bit 6 so that PAD5 will not be considered when checking
    // if the set of PADs detecting water represent a valid case.
    wsData.padStats.submergedPadsBitMask &= ~(1 << PAD5);

    // Based on the submergedPadsBitMask value, determine if it is a valid measurement
    switch (wsData.padStats.submergedPadsBitMask) {
    case 0x0:
        break;

    // Below are the valid PAD masks that represents readings from Pad4 up
    // with no gaps. Note that Pad-4 is the 0x10 mask.
    // Bits must be set in sequential logical bit sequence "down" from Pad4.
    // For example, it can't have Pad3 on without Pad4 on.  Pad2 can't
    // be set without Pad4 and Pad3, etc.
    // Note - PAD5 has been removed from the possible detected PAD bit masks
    case 0x10:  // PAD 4 is detecting water
    case 0x18:  // PADS 4,3 are detecting water
    case 0x1c:  // PADS 4,3,2 are detecting water
    case 0x1e:  // PADS 4,3,2,1 are detecting water
    case 0x1f:  // PADS 4,3,2,1,0 are detecting water
        // A valid PAD sequence has been detected
        break;

    default:
        {
            // The sensors did not detect in a logical order

            // We don't want to use the unknown case for flow rate accumulation.
            // Set the highestPad such that the flow rate lookup
            // value will be zero (in the flow rate table).
            highestPad = TOTAL_PADS;

            // Log the error case.
            // Don't roll-over the count
            uint16_t unknowsCount = wsData.padStats.unknowns;
            if (unknowsCount != (uint16_t)0xFFFF) {
                wsData.padStats.unknowns++;
            }
        }
        break;
    }

    // Return mL flow rate for this second
    return highMarkFlowRates[highestPad];
}

/**
* \brief Handle high freq to low freq meas change.  Checks if 
*        the rate at which measurements are made should move
*        from high freq rate to a low freq rate.
*/
static void doHighFreqReading(void) {
    if (wsData.padStats.lastMeasFlowRateInMl > 0) {
        wsData.padStats.secondsOfNoWater = 0;
        // no delay until next meas
        wsData.senseDelayUntilNextMeas = 0;
    } else {
        wsData.padStats.secondsOfNoWater++;
    }

    // Check if we should move to low frequency measurements
    if (wsData.padStats.secondsOfNoWater > NO_WATER_HF_TO_LF_TIME_IN_SECONDS) {
        wsData.nextWaterState = WATER_STATE_INSTALLED_LF;
        wsData.senseDelayUntilNextMeas = TIME_20_SECONDS;
    }
}

/**
* \brief Handle low freq to high freq meas change.  Checks if 
*        the rate at which measurements are made should move
*        from low freq rate to a high freq rate.
*/
static void doLowFreqReading(void) {
    // If any pad sees water, then move to the high freq meas state.
    if (wsData.padStats.lastMeasFlowRateInMl > 0) {
        wsData.padStats.secondsOfNoWater = 0;
        wsData.nextWaterState = WATER_STATE_INSTALLED_HF;
        // no delay until next meas
        wsData.senseDelayUntilNextMeas = 0;
    } else {
        // delay until next meas
        wsData.senseDelayUntilNextMeas = TIME_20_SECONDS;
    }
}

/**
* \brief This functions handles the job of switching out the Pad 
*        MAX tables.  There are two tables that get swapped
*        periodically (current and next).  The tables hold the
*        latest max capacitance measurement values seen for each
*        pad during its pre-fill or active period.  The next
*        table starts pre-filling so many minutes before the
*        current MAX table's active time is up.
*/
static void check_calibration(void) {
    volatile int i;

    // We need at least 5 minutes of continuous no water flowing before 
    // considering the next set of maximum pad values to be valid.
    if (wsData.padStats.secondsOfNoWater > 0) {
        if (wsData.validateMaxTableCounter < TIME_5_MINUTES) {
            wsData.validateMaxTableCounter++;
            if (wsData.validateMaxTableCounter >= TIME_5_MINUTES) {
                wsData.okToSwitchMaxTables = true;
            }
        }
    } else {
        wsData.validateMaxTableCounter = 0;
    }

    // We only swap max tables under certain conditions.
    // Check if 20 minutes has passed since the last swap.
    // If so, check if the next max table has been validated.
    if (((wsData.lastCalibrationTime + TIME_20_MINUTES) <= getSecondsSinceBoot())
        && wsData.okToSwitchMaxTables) {
        if (wsData.pad_max_currentP == &wsData.padStats.pad_max1_array[0]) {
            wsData.pad_max_currentP = &wsData.padStats.pad_max2_array[0];
            wsData.pad_max_nextP = &wsData.padStats.pad_max1_array[0];
        } else {
            wsData.pad_max_currentP = &wsData.padStats.pad_max1_array[0];
            wsData.pad_max_nextP = &wsData.padStats.pad_max2_array[0];
        }

        for (i = 0; i < TOTAL_PADS; i++) {
            wsData.pad_max_nextP[i] = 0;
        }

        wsData.lastCalibrationTime = getSecondsSinceBoot();
        wsData.okToSwitchMaxTables = false;
        wsData.validateMaxTableCounter = 0;
    }
    // Pre-fill start check:
    // Check if 10 minutes has passed since last calibration
    // If so, start collecting max on each pad
    if ((wsData.lastCalibrationTime + TIME_10_MINUTES) <= getSecondsSinceBoot()) {
        wsData.prefillNextMaxTableFlag = true;
    } else {
        wsData.prefillNextMaxTableFlag = false;
    }
}

#if (PERFORM_TEMPERATURE_MEAS==1)
/**
* @brief Read the internal temp sensor ADC and convert to degrees celcius.
*
* @note Temperature processing code copied from:
* http://forum.43oh.com/topic/1954-using-the-internal-temperature-sensor/
* 
* @return int Degrees C
*/
static int readInternalTemperature(void) {
    unsigned adc = 0;
    int c;
    int i;
    volatile uint16_t regVal;
    volatile uint16_t timeoutCount;

    // Configure ADC
    ADC10CTL0 = 0;
    ADC10CTL1 = INCH_10 | ADC10DIV_3;
    ADC10CTL0 = SREF_1 | ADC10SHT_3 | REFON | ADC10ON;

    // Take four readings of the ADC and average
    adc = 0;
    for (i = 0; i < 4; i++) {
        ADC10CTL0 &= ~ADC10IFG;         // Clear conversion complete flag
        ADC10CTL0 |= (ENC | ADC10SC);   // Begin ADC conversion
        timeoutCount = ~0x0;
        while (timeoutCount--) {
            regVal = ADC10CTL0;
            if (regVal & ADC10IFG) {
                break;
            }
        }
        adc += ADC10MEM;                // Read ADC
    }

    // Average
    adc >>= 2;
    // Convert to temperature for Vref = 1.5V
    c = ((27069L * adc) -  18169625L) >> 16;

    return c;
}
#endif
