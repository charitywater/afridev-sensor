/* ============================================================================ */
/* Copyright (c) 2013, Texas Instruments Incorporated                           */
/*  All rights reserved.                                                        */
/*                                                                              */
/*  Redistribution and use in source and binary forms, with or without          */
/*  modification, are permitted provided that the following conditions          */
/*  are met:                                                                    */
/*                                                                              */
/*  *  Redistributions of source code must retain the above copyright           */
/*     notice, this list of conditions and the following disclaimer.            */
/*                                                                              */
/*  *  Redistributions in binary form must reproduce the above copyright        */
/*     notice, this list of conditions and the following disclaimer in the      */
/*     documentation and/or other materials provided with the distribution.     */
/*                                                                              */
/*  *  Neither the name of Texas Instruments Incorporated nor the names of      */
/*     its contributors may be used to endorse or promote products derived      */
/*     from this software without specific prior written permission.            */
/*                                                                              */
/*  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" */
/*  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,       */
/*  THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR      */
/*  PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR            */
/*  CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,       */
/*  EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,         */
/*  PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; */
/*  OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,    */
/*  WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR     */
/*  OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,              */
/*  EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.                          */
/* ============================================================================ */

/******************************************************************************/
/* lnk_msp430g2553.cmd - LINKER COMMAND FILE FOR LINKING MSP430G2553 PROGRAMS     */
/*                                                                            */
/*   Usage:  lnk430 <obj files...>    -o <out file> -m <map file> lnk.cmd     */
/*           cl430  <src files...> -z -o <out file> -m <map file> lnk.cmd     */
/*                                                                            */
/*----------------------------------------------------------------------------*/
/* These linker options are for command line linking only.  For IDE linking,  */
/* you should set your linker options in Project Properties                   */
/* -c                                               LINK USING C CONVENTIONS  */
/* -stack  0x0100                                   SOFTWARE STACK SIZE       */
/* -heap   0x0100                                   HEAP AREA SIZE            */
/*                                                                            */
/*----------------------------------------------------------------------------*/

__Flash_Start = 0xC000;               /* Start of Flash */
__Flash_End = 0xFFFF;                 /* End of Flash */
__Boot_Start = 0xF000;                /* Boot flash */
__Boot_Reset = 0xFFFE;                /* Boot reset vector */
__Boot_VectorTable = 0xFFE0;          /* Boot vector table */

_App_Start = (__Flash_Start);         /* Start of application area */
_App_End = (__Boot_Start-1);          /* End of application area (before boot)*/
_App_Reset_Vector = (__Boot_Start-2); /* Address of Application reset vector  */
/* The app proxy interrupt table is 48 bytes long (0x30) */
/* It is stored immediately below the placed app reset vector */ 
_App_Proxy_Vector_Start = (_App_Reset_Vector-0x30); 

/****************************************************************************/
/* SPECIFY THE SYSTEM MEMORY MAP                                            */
/****************************************************************************/

MEMORY
{
    SFR                     : origin = 0x0000, length = 0x0010
    PERIPHERALS_8BIT        : origin = 0x0010, length = 0x00F0
    PERIPHERALS_16BIT       : origin = 0x0100, length = 0x0100
    RAM                     : origin = 0x0200, length = 0x01B0
    STACK                   : origin = 0x03B0, length = 0x0050
    INFOA                   : origin = 0x10C0, length = 0x0040
    INFOB                   : origin = 0x1080, length = 0x0040
    INFOC                   : origin = 0x1040, length = 0x0040
    INFOD                   : origin = 0x1000, length = 0x0040
    FLASH_WEEK1_DATA        : origin = 0xC000, length = 0x0400
    FLASH_WEEK2_DATA        : origin = 0xC400, length = 0x0400
    FLASH                   : origin = 0xC800, length = 0x27CE 
    // Interrupt Proxy table from  _App_Proxy_Vector_Start->(RESET-1)
    APP_PROXY_VECTORS       : origin = 0xEFCE, length = 48
    // App reset from _App_Reset_Vector
    RESET                   : origin = 0xEFFE, length = 0x0002
}

/****************************************************************************/
/* SPECIFY THE SECTIONS ALLOCATION INTO MEMORY                              */
/****************************************************************************/

SECTIONS
{
    .bss        : {} > RAM                /* GLOBAL & STATIC VARS              */
    .data       : {} > RAM                /* GLOBAL & STATIC VARS              */
    .sysmem     : {} > RAM                /* DYNAMIC MEMORY ALLOCATION AREA    */
	.commbufs   : {} type=NOINIT > RAM (HIGH)  /* OUTPOUR COMM BUFS            */
    .stack      : {} > STACK (HIGH)       /* SOFTWARE SYSTEM STACK             */

    .week1Data  : {} type=NOINIT > FLASH_WEEK1_DATA    /* Outpour Water Logs   */
    .week2Data  : {} type=NOINIT > FLASH_WEEK2_DATA    /* Outpour Water Logs   */

    .text       : {} > FLASH              /* CODE                              */
    .cinit      : {} > FLASH              /* INITIALIZATION TABLES             */
    .const      : {} > FLASH              /* CONSTANT DATA                     */
    .cio        : {} > RAM                /* C I/O BUFFER                      */

    .pinit      : {} > FLASH              /* C++ CONSTRUCTOR TABLES            */
    .init_array : {} > FLASH              /* C++ CONSTRUCTOR TABLES            */
    .mspabi.exidx : {} > FLASH            /* C++ CONSTRUCTOR TABLES            */
    .mspabi.extab : {} > FLASH            /* C++ CONSTRUCTOR TABLES            */

    .infoA     : {} > INFOA               /* MSP430 INFO FLASH MEMORY SEGMENTS */
    .infoB     : {} > INFOB
    .infoC     : {} > INFOC
    .infoD     : {} > INFOD

    .APP_PROXY_VECTORS : {} > APP_PROXY_VECTORS /* INTERRUPT PROXY TABLE       */
    .reset             : {} > RESET             /* MSP430 RESET VECTOR         */
}

/****************************************************************************/
/* INCLUDE PERIPHERALS MEMORY MAP                                           */
/****************************************************************************/

-l msp430g2553.cmd

