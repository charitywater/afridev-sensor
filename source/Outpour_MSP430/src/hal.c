/** 
 * @file hal.c
 * \n Source File
 * \n Outpour MSP430 Firmware
 * 
 * \brief Hal support routines
 */

#include "outpour.h"

/**
* \brief One time init of all clock related subsystems after 
*        boot up.
* \ingroup PUBLIC_API
*/
void clock_init(void) {
    DCOCTL = 0;                        // Select lowest DCOx and MODx settings
    BCSCTL1 = CALBC1_1MHZ;             // Set DCO
    DCOCTL = CALDCO_1MHZ;

    BCSCTL1 |= DIVA_0;                 // ACLK/1 [ACLK/(0:1,1:2,2:4,3:8)]
    BCSCTL2 = 0;                       // SMCLK [SMCLK/(0:1,1:2,2:4,3:8)]
    BCSCTL3 |= LFXT1S_0 | XCAP_2;      // LFXT1S0 32768-Hz crystal on LFXT1
}

/**
* \brief One time init of all gpio port pin related items after 
*        boot up.
* \ingroup PUBLIC_API
*/
void pin_init(void) {

    // I/O Control for UART
    P1SEL  |= RXD + TXD; // P1.1 = RXD, P1.2=TXD
    P1SEL2 |= RXD + TXD; // P1.1 = RXD, P1.2=TXD

    P1DIR  |= GSM_EN + GSM_DCDC + LS_VCC;
    P1OUT  &= 0x00;

    P2DIR |= BIT7; // P2.0-2.5 are pinosc, 6/7 are XIN/XOUT
    P2OUT &= 0x00; // All P2.x reset
    P2SEL |= BIT6 + BIT7;

    P3DIR |= 0xFF; // All P3.x outputs
    P3OUT &= 0x00; // All P3.x reset
}

/**
* \brief One time init of the Uart subsystem after boot up.
* \ingroup PUBLIC_API
*/
void uart_init(void) {
    //  ACLK source for UART
    UCA0CTL1 |= UCSSEL_1; // ACLK
    UCA0BR0 = 0x03; // 32 kHz 9600
    UCA0BR1 = 0x00; // 32 kHz 9600
    UCA0MCTL = UCBRS0 + UCBRS1; // Modulation UCBRSx = 3

    UCA0CTL1 &= ~UCSWRST; // **Initialize USCI state machine**
    UC0IE |= UCA0RXIE; // Enable USCI_A0 RX interrupt
}

