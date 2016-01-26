/** 
 * @file main.c
 * \n Source File
 * \n Outpour MSP430 Bootloader Firmware
 * 
 * \brief Main entry and init routines
 */

// Includes
#include "outpour.h"

/***************************
 * Module Data Definitions
 **************************/

/**
 * \def SOS_DELAY_TICKS
 * \brief Specify how long to wait until trying the next 
 *        firmware upgrade attempt.  This delay is used if the
 *        current upgrade fails, and no good code is detected in
 *        the application area.
 */
#define SOS_DELAY_TICKS ((sys_tick_t)2*60*60*12)

/***************************
 * Module Data Declarations
 **************************/

/**
* \var sosDelayTicks
* \brief Counter for the 12 hour delay used to wait until trying
*        the next firmware upgrade attempt.
*/
static uint32_t sosDelayTicks;

/**
* \var ramBlr
* \brief Scratch structure for intermediate updates to the
*        BootRecord structure.
*/
static bootloaderRecord_t ramBlr;

/**************************
 * Module Prototypes
 **************************/
static void initBootloaderRecord(void);
static int getBootloaderRecordCount(void);
static void incrementBootloaderRecordCount(void);
static bool checkForApplicationRecord(void);
static void low_power_12_hour_delay(void);

/***************************
 * Module Public Functions 
 **************************/

/**
* \brief "C" entry point
* 
* @return int Ignored
*/
int main(void) {

    // Store the status of the reboot reason (POR, Watchdog, Reset, etc).
    volatile uint8_t ifg1 = IFG1;
    // Clear the reboot reason status located in the flag register
    IFG1 = 0;

    // (Re)start and tickle the watchdog.  The watchdog is initialized for a
    // one second timeout.
    WATCHDOG_TICKLE();

    // The fwUpdateResult is the status set by the firmware upgrade state machine
    // which is located in the msgOta.c module.  The status is used
    // to make decisions on how to proceed.  
    fwUpdateResult_t fwUpdateResult = RESULT_NO_FWUPGRADE_PERFORMED;
    bool sosFlag = false;
    bool jumpToApplication = false;
    volatile int16_t blrRecordCount = 0;

    // Perform Hardware initialization
    clock_init();
    pin_init();
    uart_init();

    WATCHDOG_TICKLE();

    // Perform module initialization
    timerA0_0_init_for_sys_tick();
    modemCmd_init();
    modemLink_init();
    modemMgr_init();
    otaMsgMgr_init();

    WATCHDOG_TICKLE();

    // Increment the bootloaderRecord Count.  The bootloader record
    // is located in the flash INFO section.  Inside the structure is a 
    // counter tracking how many times we have booted since identifying 
    // an Application Record.  The bootloader uses the existance of a
    // valid Application Record to verify that the application ran 
    // successfully on the previous boot.
    // 
    // The count is zero'd below if a valid Application Record is found 
    // (the Application Record is located in one of the INFO sections).
    // 
    // If no valid bootloader record is found, a fresh bootloader 
    // structure is written.
    blrRecordCount = getBootloaderRecordCount();
    if (blrRecordCount < 0) {
        // No bootloader record was found.  This is probably because we are
        // booting for the first time after the flash was programmed and the
        // bootrecord is all FF's.
        initBootloaderRecord();
    } else {
        // Only increment the bootloader record if this is not a
        // power-on-reset (POR) or hard reset via the reset pin.
        if (!(ifg1 & (PORIFG | RSTIFG))) {
            incrementBootloaderRecordCount();
        }
    }

    // Enter the boootloader infinite loop.
    while (1) {

        // Check if a firmware upgrade message is available from the modem.
        // This is always done for every boot.  If the sosFlag is set to true,
        // a special SOS message is transmitted from the modem to the cloud in
        // addition to checking for a firmware upgrade message.
        otaMsgMgr_getAndProcessOtaMsgs(sosFlag);

        // Run in a loop until the modem check is complete.
        while (!otaMsgMgr_isOtaProcessingDone()) {
            // A "spinning" delay is used on each iteration.  The delay uses
            // the hardware timer to determine when the delay interval is complete.
            while (timerA0_0_check_for_sys_tick() == false) {
                // Polling is used for UART communication instead of interrupts.
                // So we need to run the polling function during the delay.
                modemCmd_pollUart();
                WATCHDOG_TICKLE();
            }
            // Run each exec routine that is used as part of the modem communication
            // and OTA processing.
            modemCmd_exec();
            modemLink_exec();
            modemMgr_exec();
            otaMsgMgr_exec();
        }

        // For debug
        // SendReasonAndCountToUart();

        // Get the result of the firmware upgrade check from the modem,
        // and act on it accordingly.
        fwUpdateResult = otaMsgMgr_getFwUpdateResult();
        if (fwUpdateResult == RESULT_DONE_SUCCESS) {
            // The application firmware was updated successfully.
            // Refresh the bootloader record and set to the flash to jump
            // to the application.
            initBootloaderRecord();
            // Since we just burned a new app, erase the application record.
            // The app will re-write it once it starts.  This is how we know the
            // app started correctly the next time we boot.
            msp430Flash_erase_segment(APR_LOCATION);
            jumpToApplication = true;
        } else if (fwUpdateResult == RESULT_NO_FWUPGRADE_PERFORMED) {
            // No upgrade firmware message was found in the modem.
            // We check if there is a valid application record in flash.  This
            // signals that the application code ran successfully on the previous
            // session.
            bool appRecordFound = checkForApplicationRecord();
            if (appRecordFound) {
                // If an appRecord was found, we assume the app ran OK previoulsy,
                // and we are rebooting for valid reasons.  Refresh the bootloader
                // record so that the count will be zero.
                initBootloaderRecord();
                jumpToApplication = true;
            } else {
                // There is no appRecord!
                // This could be the first time booting after the flash was
                // programmed externally.  If this is the case, then the
                // blrRecordCount will be zero.  But if the bootloader count
                // is greater than zero, then we assume something is wrong.
                // If the bootloader is greater than zero, than we assume that
                // we attempted to start the app but for some reason it did not
                // correctly write an application record. For this case, we don't
                // jump to the application.  Rather we will go into SOS mode.
                blrRecordCount = getBootloaderRecordCount();
                if (blrRecordCount == 0) {
                    jumpToApplication = true;
                }
            }
        }

        if (jumpToApplication) {
            // Get the flash parameters from the linker file.
            uint16_t app_reset_vector = getAppResetVector();
            uint16_t app_flast_start = getAppFlashStartAddr();
            uint16_t app_vector_table = getAppVectorTableAddr();
            // Verify that the application reset vector falls into a legal range
            if ((app_reset_vector >= app_flast_start) && (app_reset_vector < app_vector_table)) {
                // Start the watchdog
                WATCHDOG_TICKLE();
                // Disable Interrupts
                disableGlobalInterrupt();
                // Stop timer
                timerA0_0_halt ();
                // Jump to app
                TI_MSPBoot_APPMGR_JUMPTOAPP();
            }
        }

        // If we just completed the loop sequence with the SOS flag set, then
        // fall into a 12 hour delay.
        if (sosFlag) {
            low_power_12_hour_delay();
        } else {
            // We made it here without jumping to the application.  So something
            // is wrong. For various potential reasons, the bootloader failed to
            // upgrade successfully or identify a valid application program in flash.
            // Set the SOS flag which will result in a SOS message to be transmitted
            // to the cloud on the next loop iteration when checking for an upgrade
            // message from the modem.
            sosFlag = true;
        }
    }
}

/**********************
 * Private Functions 
 **********************/

/**
* \brief Wait for 12 hours.  The function loops for 12 hours, 
*        waking from the timer every .5 seconds to tickle the
*        watchdog.  Once the 12 hours has completed, the
*        function reboots the MSP430.
*/
static void low_power_12_hour_delay(void) {
    sosDelayTicks = SOS_DELAY_TICKS;
    timerA0_0_init_for_sleep_tick();
    // Enable the global interrupt
    enableGlobalInterrupt();
    while (sosDelayTicks) {
        WATCHDOG_TICKLE();
        // sleep, wake on Timer1A interrupt
        __bis_SR_register(LPM3_bits);
        sosDelayTicks--;
    }
    disableGlobalInterrupt();
    // Force watchdog reset
    WDTCTL = 0xDEAD;
    while (1);
}

/**
* \brief Return the count located in the Bootloader Record.  The 
*        bootloader record is a data structure located at INFO B
*        section (Flash address 0x1080).  The structure contains
*        a counter that identifies the number of boot attempts
*        since a valid application last ran.
* 
* @return int The count in the BLR data structure.  If the data
*         structure is invalid, it returns -1.
*/
static int getBootloaderRecordCount(void) {
    int blrCount = -1;
    bootloaderRecord_t *blrP = (bootloaderRecord_t *)BLR_LOCATION;
    unsigned int crc16 = gen_crc16(BLR_LOCATION, (sizeof(bootloaderRecord_t) - sizeof(uint16_t)));
    // Verify the CRC16 in the data structure.
    if (crc16 == blrP->crc16) {
        blrCount = blrP->bootRetryCount;
    }
    return blrCount;
}

/**
* \brief Write a fresh bootloader record data structure to the 
*        INFO flash location.  The count is set to zero.
*/
static void initBootloaderRecord(void) {
    // init the local RAM structure.
    memset(&ramBlr, 0, sizeof(bootloaderRecord_t));
    ramBlr.magic = BLR_MAGIC;
    ramBlr.fwVersion = ((FW_VERSION_MINOR << 8) | FW_VERSION_MAJOR);
    ramBlr.crc16 = gen_crc16((uint8_t *)&ramBlr, (sizeof(bootloaderRecord_t) - sizeof(uint16_t)));
    msp430Flash_erase_segment(BLR_LOCATION);
    msp430Flash_write_bytes(BLR_LOCATION, (uint8_t *)&ramBlr, sizeof(bootloaderRecord_t));
}

/**
* \brief Increment the count variable in the bootloader data 
*        structure located in the flash INFO section.
*/
static void incrementBootloaderRecordCount(void) {
    memcpy(&ramBlr, BLR_LOCATION, sizeof(bootloaderRecord_t));
    ramBlr.bootRetryCount++;
    ramBlr.crc16 = gen_crc16((uint8_t *)&ramBlr, (sizeof(bootloaderRecord_t) - sizeof(uint16_t)));
    msp430Flash_erase_segment(BLR_LOCATION);
    msp430Flash_write_bytes(BLR_LOCATION, (uint8_t *)&ramBlr, sizeof(bootloaderRecord_t));
}

/**
* \brief Determine if a valid Application Record is located in 
*        the INFO C section.  The record is written by the
*        Application and used by the bootloader to identify that
*        the Application ran successfully in the previous
*        session.
* 
* @return bool Returns true if the record is valid.  False 
*         otherwise.
*/
static bool checkForApplicationRecord(void) {
    bool aprFlag = false;
    appRecord_t *aprP = (appRecord_t *)APR_LOCATION;
    unsigned int crc16 = gen_crc16(APR_LOCATION, (sizeof(appRecord_t) - sizeof(uint16_t)));
    if (crc16 == aprP->crc16) {
        aprFlag = true;
    }
    return aprFlag;
}

/** 
 * \var Vector_Table 
 * \brief The MSPBoot Vector Table is fixed and it can't be 
 *        erased or modified. Most vectors point to a proxy
 *        vector entry in Application area.  The bootloader only
 *        uses one interrupt - the timer interrupt.
 *  
 *  The MSP430 uses dedicated hardware interrupt vectors.  The
 *  CPU grabs the address at the vector location and jumps it.
 *  Because these locations are fixed according to the CPU
 *  design, a proxy method must be used so that the Application
 *  can use interrupts based on its own needs. Each vector in
 *  this table is setup so that it points to a special location
 *  in the Application code.  The special location in the
 *  application code contains a branch instruction to the
 *  specific application interrupt handler.
 */

//
//  Macros and definitions used in the interrupt vector table
//
/* Value written to unused vectors */
#define UNUSED                  (0x3FFF)
/*! Macro used to calculate address of vector in Application Proxy Table */
#define APP_PROXY_VECTOR(x)     ((uint16_t)&_App_Proxy_Vector_Start[x*2])

#ifdef __IAR_SYSTEMS_ICC__
#pragma location="BOOT_VECTOR_TABLE"
__root const uint16_t Vector_Table[] =
#elif defined (__TI_COMPILER_VERSION__)
#pragma DATA_SECTION(Vector_Table, ".BOOT_VECTOR_TABLE")
#pragma RETAIN(Vector_Table)
const uint16_t Vector_Table[] =
#endif
{
    UNUSED,                                     // FFE0 = unused
    UNUSED,                                     // FFE2 = unused
    APP_PROXY_VECTOR(0),                        // FFE4 = P1
    APP_PROXY_VECTOR(1),                        // FFE6 = P2
    UNUSED,                                     // FFE8 = unused
    APP_PROXY_VECTOR(2),                        // FFEA = ADC10
    APP_PROXY_VECTOR(3),                        // FFEC = USCI I2C TX/RX
    APP_PROXY_VECTOR(4),                        // FFEE = USCI I2C STAT
    APP_PROXY_VECTOR(5),                        // FFF0 = TA0_1
    (uint16_t)ISR_Timer0_A0,                    // FFF2 = TA0_0
    APP_PROXY_VECTOR(7),                        // FFF4 = WDT
    APP_PROXY_VECTOR(8),                        // FFF6 = COMP_A
    APP_PROXY_VECTOR(9),                        // FFF8 = TA1_1
    APP_PROXY_VECTOR(10),                       // FFFA = TA1_0
    APP_PROXY_VECTOR(11),                       // FFFC = NMI
};

#if 0
// DEBUG FUNCTION
volatile uint8_t ifg1;
volatile int16_t blrRecordCount;
#pragma SET_CODE_SECTION(".infoD")
void SendReasonAndCountToUart(void) {
    volatile int i;
    for (i = 0; i < 20; i++) {
        _delay_cycles(10000);
        UCA0TXBUF = ifg1;
        _delay_cycles(10000);
        UCA0TXBUF = blrRecordCount;
    }
}
#pragma SET_CODE_SECTION()
#endif

