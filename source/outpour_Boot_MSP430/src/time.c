/** 
 * @file time.c
 * \n Source File
 * \n Outpour MSP430 Bootloader Firmware
 * 
 * \brief MSP430 timer control and support routines
 */

#include "outpour.h"

/**
* \var seconds_since_boot
* \brief Declare seconds since boot.  Incremented by timer 
*        interrupt routine.
*/
static volatile uint32_t sysTicks_since_boot = 0;

/**
* \brief Retrieve the number of system ticks since the system 
*        booted.
* 
* @return uint32_t 32 bit values representing the system tick 
*         count since the system booted.
*/
uint32_t getSysTicksSinceBoot(void) {
    return sysTicks_since_boot;
}

/*
*  Note - the timer naming convention is very confusing on the MSP430
*
*  For the MSP430G2553, there are two timers on the system:  TimerA0 and TimerA1
*    Each timer has two capture & control channels: 0 and 1
* 
*  Naming Usage:
*  TimerA0_0, Timer A0, capture control channel 0
*  TimerA0_1, Timer A0, capture control channel 1
*  TimerA1_1, Timer A1, capture control channel 0
*  TimerA1_1, Timer A1, capture control channel 1
*
*  Timer allocation/usage for Outpour APPLICATION:
*    A0, Capture control channel 0 used with capacitance reading (no ISR)
*    A1, Capture control channel 0 used for system tick (with ISR, vector = 13 @ 0FFFAh)
*
*  Timer allocation/usage for Outpour BOOT:
*    A0, Capture control channel 0 used for system tick (with ISR, vector = 9 @ 0FFF2h)
* 
*  The interrupt vector naming corresponds as follows:
* TIMER0_A0_VECTOR -> Timer A0, capture/control channel 0, vector 9, 0xFFF2
* TIMER0_A1_VECTOR -> Timer A0, capture/control channel 1, vector 8, 0xFFF0
* TIMER1_A0_VECTOR -> Timer A1, capture/control channel 0, vector 13, 0xFFFA
* TIMER1_A1_VECTOR -> Timer A1, capture/control channel 1, vector 12, 0xFFF8
*
* =============================================================================== 
* Timer Registers for Timer A0 and A1  (x = 0 or 1 accordingly)
*  TAxCTL - reg to setup timer counting type, clock, etc - CONTROLS ALL OF TIMER A1
*  Capture/Control channel 0
*  TAxCCR0 - specify the compare count for channel 0
*  TAxCCTL0 - setup interrupt for channel 0 
*  Capture/Control channel 1
*  TAxCCR1 - specify the compare count for channel 1
*  TAxCCTL1 - setup interrupt for channel 1
*/

/**
* \brief Initialize and start timerA0 for the short system tick. 
*        Use capture control channel 0 (timerA0_0)
*/
void timerA0_0_init_for_sys_tick(void) {
    // Halt the timer A0
    TA0CTL = 0;
    // Set up TimerA0_0 to set interrupt flag every 0.03125 seconds, 
    // counting to 2^10 aclk cycles
    TA0CCR0 = 0x400;
    // Clear all of TimerA0_0 control
    TA0CCTL0 = 0;
    // Start TimerA0 using ACLK, UP MODE, Input clock divider = 1
    TA0CTL = TASSEL_1 + MC_1 + TACLR;
}

/**
 * \brief Returns true if a compare event has occured. If the
 *        interrupt flag is set, the interrupt flag is cleared.
 */
bool timerA0_0_check_for_sys_tick(void) {
    // If the interrupt flag for TimerA0_0 is not set, just return
    if ((CCIFG&TA0CCTL0)==0) {
        return false;
    }
    // Clear TimerA0_0 interrupt flag
    TA0CCTL0 &= ~CCIFG;
    sysTicks_since_boot++;
    return true;
}

/**
* \brief Initialize and start timerA0 for the long system tick.
*/
void timerA0_0_init_for_sleep_tick (void) {
    // Halt the timer A0
    TA0CTL = 0;
    // Set up TimerA0_0 to set interrupt flag every 0.5 seconds, 
    // counting to 2^14 aclk cycles
    // For 12 hour SOS
    TA0CCR0 = 0x4000;  // 16384 * 1/32768 = 0.5 seconds
    // For 5 minute SOS (DEBUG ONLY)
    // TA0CCR0 = 114;  // 0x72 [114*(1/32768) = 0.003479 seconds]
    // Clear all of TimerA0_0 channel 1 control
    TA0CCTL0 = 0;
    // Start TimerA0 using ACLK, UP MODE, Input clock divider = 1
    // TA0CTL = TASSEL_1 + MC_1 + TACLR + ID_3;
    TA0CTL = TASSEL_1 + MC_1 + TACLR + ID_0;
    // Enable timer TimerA0_0 to interrupt the CPU 
    TA0CCTL0 |= CCIE;
}

/**
 * \brief TimerA0_0 ISR for Capture/Control channel 1.
 */
#pragma vector=TIMER0_A0_VECTOR
__interrupt void ISR_Timer0_A0(void) {
    // Clear TimerA0_0 interrupt flag
    TA0CCTL0 &= ~CCIFG;
    __bic_SR_register_on_exit(LPM3_bits);
}

