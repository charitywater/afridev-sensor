/** 
 * @file main.c
 * \n Source File
 * \n Outpour MSP430 Firmware
 * 
 * \brief Main entry and init routines
 */

// Includes
#include "outpour.h"

/***************************
 * Module Data Definitions
 **************************/

/**************************
 * Module Prototypes
 **************************/

/***************************
 * Module Public Functions 
 **************************/

/**
* \brief "C" entry point
* 
* @return int Ignored
*/
int main(void) {

    WDTCTL = WDTPW | WDTHOLD;   // Stop watchdog timer

    // Perform Hardware Initialization
    clock_init();
    pin_init();
    uart_init();

    // Call sys exec and never return.
    sysExec_exec();

    // System should never reach this point in execution.
    // But just in case.....
    disableGlobalInterrupt();
    // Force watchdog reset
    WDTCTL = 0xDEAD;
    while (1);
}

/**********************
 * Private Functions 
 **********************/

/**
* \brief sys_error
* \brief A catastrophic situation has occured.  Restart system.
*/
void sysError(void) {
#if 0
    // For development, loop infinitely.
    while (1);
#else
    // Reset system
    // Force watchdog reset
    WDTCTL = 0xDEAD;
    while (1);
#endif
}

#ifdef FOR_USE_WITH_BOOTLOADER

//
//  External ISR prototypes used by ProxyVectorTable
//
extern __interrupt void ISR_Timer1_A0(void);
extern __interrupt void USCI0TX_ISR(void);
extern __interrupt void USCI0RX_ISR(void);
extern __interrupt void watchdog_timer(void);

/******************************************************************************
 *
 * @brief   Dummy Interrupt service routine
 *  This ISR will be executed if an undeclared interrupt is called
 *
 * @return  none
 *****************************************************************************/
__interrupt void Dummy_Isr(void)
{
    // Something catastrophic has happened.
    // Erase the application record so that the bootloader will go into SOS mode.
    msp430Flash_erase_segment(APR_LOCATION);
    // Force watchdog reset
    WDTCTL = 0xDEAD;
    while(1);
}

/**
 * \var ProxyVectorTable
 *
 * \brief This is a "proxy" interrupt table which is used by the
 *         bootloader to jump to each interrupt routine in the
 *         application.  This technique is based on a TI
 *         bootloader design.  Please see document SLAA600A for
 *         a description of this scheme.  Note that if an
 *         interrupt vector is not used by the application, the
 *         Dummy_Isr function pointer is used.  Currently the
 *         only interrupts used by the application are the
 *         timer and UART rx/tx.
 *
 * \brief Some details about the proxy table;
 * \li It always resides in the same location as specified in
 *     the linker command file.  The location must be known by
 *     the bootloader.
 * \li It contains a BRA instruction (0x4030) followed by the
 *     address of each ISR processing routine.  The bootloader
 *     jumps to the application ISR routine by jumping to the
 *     location associated with the hardware interrupt in the
 *     table.
 * \li Note that the number and location of each vector should
 *     match the declaration in Boot's Vector_Table
 */

#   ifdef __IAR_SYSTEMS_ICC__
#       pragma location="APP_PROXY_VECTORS"
__root const uint16_t ProxyVectorTable[] =
#   elif defined (__TI_COMPILER_VERSION__)
#       pragma DATA_SECTION(ProxyVectorTable, ".APP_PROXY_VECTORS")
#       pragma RETAIN(ProxyVectorTable)
const uint16_t ProxyVectorTable[] =
#   endif
{
    0x4030, (uint16_t) Dummy_Isr,           // APP_PROXY_VECTOR(0) P1
    0x4030, (uint16_t) Dummy_Isr,           // APP_PROXY_VECTOR(1) P2
    0x4030, (uint16_t) Dummy_Isr,           // APP_PROXY_VECTOR(2) ADC10
    0x4030, (uint16_t) USCI0TX_ISR,         // APP_PROXY_VECTOR(3) USCI I2C TX/RX
    0x4030, (uint16_t) USCI0RX_ISR,         // APP_PROXY_VECTOR(4) USCI I2C STAT
    0x4030, (uint16_t) Dummy_Isr,           // APP_PROXY_VECTOR(5) TA0_1
    0x4030, (uint16_t) Dummy_Isr,           // APP_PROXY_VECTOR(6) T0_0
    0x4030, (uint16_t) watchdog_timer,      // APP_PROXY_VECTOR(7) WDT
    0x4030, (uint16_t) Dummy_Isr,           // APP_PROXY_VECTOR(8) COMP_A
    0x4030, (uint16_t) Dummy_Isr,           // APP_PROXY_VECTOR(9) TA1_1
    0x4030, (uint16_t) ISR_Timer1_A0,       // APP_PROXY_VECTOR(10) TA1_0
    0x4030, (uint16_t) Dummy_Isr,           // APP_PROXY_VECTOR(11) NMI
};
#endif

/******************************
 * Doxygen Support Defines 
 ******************************/

/**
* \defgroup PUBLIC_API Public API
* \brief Public API's are functions called by other C modules to
*        initiate some type of work or set/get data in the
*        module.
*/

/**
* \defgroup SYSTEM_INIT_API System and Module Initialization API
* \brief These routines are called once at system start to 
*        initialize data and/or hardware.  Each main processing
*        module in the system has its own initialization
*        routine.
*/

/**
* \defgroup EXEC_ROUTINE Top Level Processing Routines.
* \brief Exec routines are called on each iteration of the main
*        processing loop for the software.  They are responsible
*        for "orchestrating" the specific functionality of a
*        specific module in the system.  Because the system does
*        not have multiple threads, all work must be done in a
*        piece meal fashion using state machine processing by
*        each Exec as needed.
*/

/**
* \defgroup ISR Interrupt Handler API
* \brief Interrupt handler for the system.  Currently there are 
*        three types used:  UART, A0 Timer and Watchdog timer.
*        \li The Uart has an individual ISR for transmit and
*        receive.
*        \li The A0 Timer interrupt is a one second tick timer
*        used to bring the system out of low power mode and
*        initiate the main loop processing.
*        \li The Watchdog timer interrupt is used as part of the
*        Capacitive touch processing.
*/

