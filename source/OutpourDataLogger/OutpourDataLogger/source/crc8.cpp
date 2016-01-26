/** 
*  @file crc8.cpp
*  Slamball PC test program for firmware development.
*  This Module contains routines to perform the crc8 algorithm 
*
*
*  WARNING:
*  This code is provided for example purpose only.  There is no guarantee for 
*  correct operation in any manner.
*/
#include <windows.h>
#include <time.h>
#include <stdio.h>
#include <string.h>
#include <conio.h>

/*******************************************************************************
*
*
*
*******************************************************************************/
#define INITIAL_REMAINDER	((unsigned char)0x0)
#define FINAL_XOR_VALUE		((unsigned char)0x0)
#define POLYNOMIAL ((unsigned char)0x1D)
#define WIDTH  (8 * sizeof(crc))
#define TOPBIT (1 << (WIDTH - 1))
typedef unsigned char crc;
crc  crcTable[256];

void crcInit (void);
crc crcFast(unsigned char const message[], int nBytes);

/*******************************************************************************
*
*
*
*******************************************************************************/
unsigned char calcCrc8 (unsigned char *buffer, int length)
{
	return crcFast(buffer, length);
}


/*******************************************************************************
*
*
*
*******************************************************************************/
void crcInit (void)
{
	crc  remainder;

	/*
	* Compute the remainder of each possible dividend.
	*/
	for (int dividend = 0; dividend < 256; ++dividend)
	{
		/*
		* Start with the dividend followed by zeros.
		*/
		remainder = dividend << (WIDTH - 8);

		/*
		* Perform modulo-2 division, a bit at a time.
		*/
		for (unsigned char bit = 8; bit > 0; --bit)
		{
			/*
			* Try to divide the current data bit.
			*/			
			if (remainder & TOPBIT)
			{
				remainder = (remainder << 1) ^ POLYNOMIAL;
			}
			else
			{
				remainder = (remainder << 1);
			}
		}

		/*
		* Store the result into the table.
		*/
		crcTable[dividend] = remainder;
	}

}   /* crcInit() */

/*******************************************************************************
*
*
*
*******************************************************************************/
crc crcFast(unsigned char const message[], int nBytes)
{
	unsigned char data;
	crc remainder = INITIAL_REMAINDER;

	/*
	* Divide the message by the polynomial, a byte at a time.
	*/
	for (int byte = 0; byte < nBytes; ++byte)
	{
		data = message[byte] ^ (remainder >> (WIDTH - 8));
		remainder = crcTable[data] ^ (remainder << 8);
	}

	/*
	* The final remainder is the CRC.
	*/
	return (remainder ^ FINAL_XOR_VALUE);

}   /* crcFast() */

/*******************************************************************************
*
*
*
*******************************************************************************/
static void test_crc8 (void)
{
	unsigned char data[] = { 0x57, 1, 3, 4 };
	unsigned char result;

	// CRC
	result = calcCrc8 ((unsigned char *)data, (unsigned char)2);

	printf ("CRC Test Result is 0x%x\n", result);
}

/****************************************************************************** 
*
*  Description:
*    Generate a crc value on the data buffer passed in.
*  Input
*    unsigned char dataP - Pointer to the buffer to calculate the crc over
*    unsigned char length - length of the total bytes to calcuate crc over
* 
*  Return
*    unsigned char - calculated crc value
* 
*******************************************************************************/
unsigned char sbg_crc8 (unsigned char*dataP, unsigned char length)
{
    unsigned char *current_dataP = dataP;
    unsigned char crc_Dbyte = 0;
    unsigned char byte_counter;
    unsigned char bit_counter;

    for (byte_counter=0; byte_counter < length; byte_counter++) {
        crc_Dbyte ^= *current_dataP;
        for (bit_counter=0; bit_counter < 8; bit_counter++) {
            if (crc_Dbyte & 0x80) {
                crc_Dbyte <<= 1;
                crc_Dbyte ^= 0x07;
            } else
                crc_Dbyte <<= 1;
        }
        current_dataP++;
    }
    return crc_Dbyte;
}
