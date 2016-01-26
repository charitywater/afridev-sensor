/* --COPYRIGHT--,BSD
 * Copyright (c) 2012, Texas Instruments Incorporated
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * *  Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * *  Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * *  Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * --/COPYRIGHT--*/
/*!
 *  @file   CTS_HAL.c
 *
 *  @brief  This file contains the source for the different implementations.
 *
 *  @par    Project:
 *             MSP430 Capacitive Touch Library 
 *
 *  @par    Developed using:
 *             CCS Version : 5.4.0.00048, w/support for GCC extensions (--gcc)
 *  \n         IAR Version : 5.51.6 [Kickstart]
 *
 *  @author  C. Sterzik
 *  @author  T. Hwang
 *
 *  @version     1.2 Added HALs for new devices.
 *
 *  @par    Supported Hardware Implementations:
 *              - TI_CTS_RO_COMPAp_TA0_WDTp_HAL()
 *              - TI_CTS_fRO_COMPAp_TA0_SW_HAL()
 *              - TI_CTS_fRO_COMPAp_SW_TA0_HAL()
 *              - TI_CTS_RO_COMPAp_TA1_WDTp_HAL()
 *              - TI_CTS_fRO_COMPAp_TA1_SW_HAL()
 *              - TI_CTS_RC_PAIR_TA0_HAL()
 *              - TI_CTS_RO_PINOSC_TA0_WDTp_HAL()
 *              - TI_CTS_RO_PINOSC_TA0_HAL()
 *              - TI_CTS_fRO_PINOSC_TA0_SW_HAL()
 *              - TI_CTS_RO_COMPB_TA0_WDTA_HAL()
 *              - TI_CTS_RO_COMPB_TA1_WDTA_HAL()
 *              - TI_CTS_fRO_COMPB_TA0_SW_HAL()
 *              - TI_CTS_fRO_COMPB_TA1_SW_HAL()
 *              - Added in version 1.1
 *              - TI_CTS_fRO_PINOSC_TA0_TA1_HAL()
 *              - TI_CTS_RO_PINOSC_TA0_TA1_HAL()
 *              - TI_CTS_RO_CSIO_TA2_WDTA_HAL()
 *              - TI_CTS_RO_CSIO_TA2_TA3_HAL()
 *              - TI_CTS_fRO_CSIO_TA2_TA3_HAL()
 *              - TI_CTS_RO_COMPB_TB0_WDTA_HAL()
 *              - TI_CTS_RO_COMPB_TA1_TA0_HAL()
 *              - TI_CTS_fRO_COMPB_TA1_TA0_HAL()
 *              - Added in version 1.2
 *              - TI_CTS_RO_PINOSC_TA1_WDTp_HAL()
 *              - TI_CTS_RO_PINOSC_TA1_TB0_HAL()
 *              - TI_CTS_fRO_PINOSC_TA1_TA0_HAL()
 *              - TI_CTS_fRO_PINOSC_TA1_TB0_HAL()
 *
 */

#include "CTS_HAL.h"

#ifdef RO_PINOSC_TA0_WDTp
/*!
 *  @brief   RO method capactiance measurement with PinOsc IO, TimerA0, and WDT+
 *
 *  \n       Schematic Description: 
 * 
 *  \n       element-----+->Px.y
 * 
 *  \n       The WDT+ interval represents the measurement window.  The number of 
 *           counts within the TA0R that have accumulated during the measurement
 *           window represents the capacitance of the element.
 * 
 *  @param   group Pointer to the structure describing the Sensor to be measured
 *  @param   counts Pointer to where the measurements are to be written
 *  @return  none
 */
void TI_CTS_RO_PINOSC_TA0_WDTp_HAL(const struct Sensor *group, uint16_t *counts) {
    uint8_t i;

//** Context Save
//  Status Register:
//  WDTp: IE1, WDTCTL
//  TIMERA0: TA0CTL, TA0CCTL1
//  Ports: PxSEL, PxSEL2
    uint8_t contextSaveSR;
    uint8_t contextSaveIE1;
    uint16_t contextSaveWDTCTL;
    uint16_t contextSaveTA0CTL, contextSaveTA0CCTL1, contextSaveTA0CCR1;
    uint8_t contextSaveSel, contextSaveSel2;

    contextSaveSR = __get_SR_register();
    contextSaveIE1 = IE1;
    contextSaveWDTCTL = WDTCTL;
    contextSaveWDTCTL &= 0x00FF;
    contextSaveWDTCTL |= WDTPW;
    contextSaveTA0CTL = TA0CTL;
    contextSaveTA0CCTL1 = TA0CCTL1;
    contextSaveTA0CCR1 = TA0CCR1;

//** Setup Measurement timer***************************************************
// Choices are TA0,TA1,TB0,TB1,TD0,TD1 these choices are pushed up into the
// capacitive touch layer.

    // Configure and Start Timer
    TA0CTL = TASSEL_3 + MC_2;             // INCLK, cont mode
    TA0CCTL1 = CM_3 + CCIS_2 + CAP;       // Pos&Neg,GND,Cap
    IE1 |= WDTIE;                         // enable WDT interrupt
    for (i = 0; i < (group->numElements); i++) {
        // Context Save
        contextSaveSel = *((group->arrayPtr[i])->inputPxselRegister);
        contextSaveSel2 = *((group->arrayPtr[i])->inputPxsel2Register);
        // Configure Ports for relaxation oscillator
        *((group->arrayPtr[i])->inputPxselRegister) &= ~((group->arrayPtr[i])->inputBits);
        *((group->arrayPtr[i])->inputPxsel2Register) |= ((group->arrayPtr[i])->inputBits);
        //**  Setup Gate Timer ********************************************************
        // Set duration of sensor measurment
        //WDTCTL = (WDTPW+WDTTMSEL+group->measGateSource+group->accumulationCycles);
        WDTCTL = (WDTPW + WDTTMSEL + (group->measGateSource) + (group->accumulationCycles));
        TA0CTL |= TACLR;                     // Clear Timer_A TAR
        if (group->measGateSource == GATE_WDT_ACLK) {
            __bis_SR_register(LPM3_bits + GIE);   // Wait for WDT interrupt
        } else {
            __bis_SR_register(LPM0_bits + GIE);   // Wait for WDT interrupt
        }
        TA0CCTL1 ^= CCIS0;           // Create SW capture of CCR1
        counts[i] = TA0CCR1;         // Save result
        WDTCTL = WDTPW + WDTHOLD;    // Stop watchdog timer
                                     // Context Restore
        *((group->arrayPtr[i])->inputPxselRegister) = contextSaveSel;
        *((group->arrayPtr[i])->inputPxsel2Register) = contextSaveSel2;
    }
    // End Sequence
    // Context Restore
    __bis_SR_register(contextSaveSR);
    if (!(contextSaveSR & GIE)) {
        __bic_SR_register(GIE);   //
    }
    IE1 = contextSaveIE1;
    WDTCTL = contextSaveWDTCTL;
    TA0CTL = contextSaveTA0CTL;
    TA0CCTL1 = contextSaveTA0CCTL1;
    TA0CCR1 = contextSaveTA0CCR1;
}
#endif

#ifdef WDT_GATE
/**
 *  ======== watchdog_timer ========
 *  @ingroup ISR
 *  @brief  WDT_ISR Used as part of the Capacitive processing.
 *          This ISR clears the LPM bits found in the Status
 *          Register which result in the processing leaving low
 *          power mode.
 */
#ifndef FOR_USE_WITH_BOOTLOADER
#pragma vector=WDT_VECTOR
#endif
__interrupt void watchdog_timer(void) {
    __bic_SR_register_on_exit(LPM3_bits);  // Exit LPM3 on reti
}
#endif

