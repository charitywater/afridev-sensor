/** 
 * @file time.c
 * \n Source File
 * \n Outpour MSP430 Firmware
 * 
 * \brief MSP430 sleep control and support routines
 */

/*
*  Note - the timer naming convention is very confusing on the MSP430
*
*  For the MSP430G2553, there are two timers on the system:  TimerA0 and TimerA1
*    Each timer has two capture & control channels: 0 and 1
* 
*  Naming Usage:
*  TimerA0_0, Timer A0, capture control channel 0
*  TimerA0_1, Timer A0, capture control channel 1
*  TimerA1_0, Timer A1, capture control channel 0
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

#include "outpour.h"
#include "RTC_Calendar.h"

#pragma NOINIT(tp)

/**
* \var tp
* \brief Internal static structure to hold the time in binary or 
*        bcd format.  Pointer to this structure is returned by
*        the getBinTime and getBcdTime functions.
*/
static timePacket_t tp;

/**
* \var seconds_since_boot
* \brief Declare seconds since boot.  Incremented by timer 
*        interrupt routine.
*/
static volatile uint32_t seconds_since_boot = 0;

/**
* \brief Initialize and start timerA1 for the one second system 
*        tick.  Uses Timer A1, capture/control channel 0, vector
*        13, 0xFFFA
* \ingroup PUBLIC_API
*/
void timerA1_init(void) {
    // Set up Timer1 to wake every second, counting to 2^15 aclk cycles
    TA1CCR0 = 0x8000;
    TA1CTL = TASSEL_1 + MC_1 + TACLR;
    TA1CCTL0 &= ~CCIFG;
    TA1CCTL0 |= CCIE;
}

/**
* \brief Retrieve the seconds since boot system value.
* \ingroup PUBLIC_API
* 
* @return uint32_t 32 bit values representing seconds since the 
*         system booted.
*/
uint32_t getSecondsSinceBoot(void) {
    return seconds_since_boot;
}

/**
* \brief Timer ISR. Produces the 1HZ system tick interrupt. 
*        Uses Timer A1, capture/control channel 0, vector 13,
*        0xFFFA
* \ingroup ISR
*/
#ifndef FOR_USE_WITH_BOOTLOADER
#pragma vector=TIMER1_A0_VECTOR
#endif
__interrupt void ISR_Timer1_A0(void) {
    TA1CTL |= TACLR;
    // Increment the TI calendar library
    incrementSeconds();
    // Increment the seconds counter
    seconds_since_boot++;
    // Because we want to wake the processor, use the following
    // to set the power control bits on the MSP430.
    __bic_SR_register_on_exit(LPM3_bits);
}

#if 0
void calibrateLoopDelay (void){
    P1DIR |= BIT3;
    while (1) {
        P1OUT &= ~BIT3;
        _delay_cycles(1000);
        P1OUT |= BIT3;
        _delay_cycles(1000);
    }
}
#endif

/**
* \brief Utility function to get the time from the TI calender 
*        module and convert to binary format.
* \note Only tens is returned for year (i.e. 15 for 2015, 16 for
*       2016, etc.)
* 
* @return timePacket_t* Pointer to a time packet structure 
*           to fill in with the time.
*/
timePacket_t* getBinTime(void) {
    uint16_t mask;
    mask = getAndDisableSysTimerInterrupt();
    tp.second  = bcd_to_char(TI_second);
    tp.minute  = bcd_to_char(TI_minute);
    tp.hour24  = bcd_to_char(get24Hour());
    tp.day     = bcd_to_char(TI_day);
    tp.month   = bcd_to_char(TI_month) + 1; // correcting from RTC lib's 0-indexed month
    tp.year    = bcd_to_char((TI_year)&0xFF);
    restoreSysTimerInterrupt(mask);
    return &tp;
}

/**
* \brief Utility function to get the time from the TI calendar
*        module in bcd format.
*  
* \note Month is returned as BCD zero based.  Only tens is 
*       returned for year (i.e. 15 for 2015, 16 for 2016, etc.)
* 
* @return timePacket_t* Pointer to a time packet structure 
*           to fill in with the time.
*/
timePacket_t* getBcdTime(void) {
    uint16_t mask;
    mask = getAndDisableSysTimerInterrupt();
    tp.second  = TI_second;
    tp.minute  = TI_minute;
    tp.hour24  = get24Hour();
    tp.day     = TI_day;
    tp.month   = TI_month; // currently 0 based
    tp.year    = ((TI_year)&0xFF);
    restoreSysTimerInterrupt(mask);
    return &tp;
}

/**
 * \brief Utility function to convert a byte of bcd data to a 
 *        binary byte.  BCD data represents a value of 0-99,
 *        where each nibble of the input byte is one decimal
 *        digit.
 * 
 * @param bcdValue Decimal value, each nibble is a digit of 0-9.
 * 
 * @return uint8_t binary conversion of the BCD value.
 *  
 */
uint8_t bcd_to_char(uint8_t bcdValue) {
    uint8_t tenHex = (bcdValue >> 4) & 0x0f;
    uint8_t tens = (((tenHex << 2) + (tenHex)) << 1);
    uint8_t ones = bcdValue & 0x0f;
    return (tens + ones);
}
