/** 
 * @file utils.c
 * \n Source File
 * \n Outpour MSP430 Firmware
 * 
 * \brief Miscellaneous support functions.
 */

#include "outpour.h"

/**
 * \def CRC16
 * \brief The CRC-16-ANSI polynomial constant.
 */
#define CRC16 0x8005

/**
* \brief Utility function to calculate a 16 bit CRC on data in a
*        buffer.
* 
* @param data Pointer to the data buffer to calculate the CRC
*             over
* @param size Length of the data in bytes to calculate over.
* 
* @return unsigned int The CRC calculated value
*/
unsigned int gen_crc16(const unsigned char *data, unsigned int size) {
    volatile unsigned int out = 0;
    volatile int bits_read = 0;
    volatile int bit_flag;

    while (size > 0) {
        bit_flag = out >> 15;
        /* Get next bit: */
        out <<= 1;
        out |= (*data >> bits_read) & 1; // item a) work from the least significant bits
        /* Increment bit counter: */
        bits_read++;
        if (bits_read > 7) {
            bits_read = 0;
            data++;
            size--;
        }
        /* Cycle check: */
        if (bit_flag) out ^= CRC16;
    }

    // item b) "push out" the last 16 bits
    unsigned int i;
    for (i = 0; i < 16; ++i) {
        bit_flag = out >> 15;
        out <<= 1;
        if (bit_flag) out ^= CRC16;
    }

    // item c) reverse the bits
    unsigned int crc = 0;
    i = 0x8000;
    unsigned int j = 0x0001;
    for (; i != 0; i >>= 1, j <<= 1) {
        if (i & out) crc |= j;
    }
    return crc;
}

/**
* \brief Utility function to calculate a 16 bit CRC on data 
*        located in two buffers. The first buffer is most likely
*        the opcode buffer, and the second buffer is the data
*        buffer.
* 
* @param data1 Pointer to first buffer
* @param size1 Size of data in bytes for first buffer
* @param data2 Pointer to data in second buffer
* @param size2 Size of data in bytes for second buffer
* 
* @return unsigned int The calculated CRC value
*/
unsigned int gen_crc16_2buf(const unsigned char *data1, unsigned int size1, const unsigned char *data2, unsigned int size2) {
    volatile unsigned int out = 0;
    volatile int bits_read = 0;
    volatile int bit_flag;

    while (size1 > 0) {
        bit_flag = out >> 15;
        /* Get next bit: */
        out <<= 1;
        out |= (*data1 >> bits_read) & 1; // item a) work from the least significant bits
        /* Increment bit counter: */
        bits_read++;
        if (bits_read > 7) {
            bits_read = 0;
            data1++;
            size1--;
        }
        /* Cycle check: */
        if (bit_flag) out ^= CRC16;
    }

    while (size2 > 0) {
        bit_flag = out >> 15;

        /* Get next bit: */
        out <<= 1;
        out |= (*data2 >> bits_read) & 1; // item a) work from the least significant bits

        /* Increment bit counter: */
        bits_read++;
        if (bits_read > 7) {
            bits_read = 0;
            data2++;
            size2--;
        }
        /* Cycle check: */
        if (bit_flag) out ^= CRC16;
    }

    // item b) "push out" the last 16 bits
    unsigned int i;
    for (i = 0; i < 16; ++i) {
        bit_flag = out >> 15;
        out <<= 1;
        if (bit_flag) out ^= CRC16;
    }

    // item c) reverse the bits
    unsigned int crc = 0;
    i = 0x8000;
    unsigned int j = 0x0001;
    for (; i != 0; i >>= 1, j <<= 1) {
        if (i & out) crc |= j;
    }
    return crc;
}


/**
* \brief Validate a Minute or Second BCD value for a legal 
*        range.
* 
* @param bcdVal The bcd value to check (0-59)
* 
* @return bool Returns true if valid.
*/
bool isBcdMinSecValValid (uint8_t bcdVal) {
    bool isOk = true;
    uint8_t binVal = bcd_to_char (bcdVal);
    if (binVal > 59) {
        isOk = false;
    }
    else if ((bcdVal & 0xF) > 9) {
        isOk = false;
    }
    else if ((bcdVal >> 4) > 5) {
        isOk = false;
    }
    return isOk;
}

/**
* \brief Validate a 24 Hour BCD value for a legal range.
* 
* @param bcdVal The bcd value to check (0-23)
* 
* @return bool Returns true if valid.
*/
bool isBcdHour24Valid (uint8_t bcdVal) {
    bool isOk = true;
    uint8_t binVal = bcd_to_char (bcdVal);
    uint8_t tens = bcdVal >> 4;
    uint8_t ones = bcdVal & 0xF;
    if (binVal > 23) {
        isOk = false;
    } else if (tens>2) {
        isOk = false;
    } else if ((tens<2)&&(ones>9)) {
        isOk = false;
    } else if ((tens==2)&&(ones>3)) {
        isOk = false;
    }
    return isOk;
}

/**
* \brief Write a fresh Application Record in the INFO C section.
*        The record is written by the Application and used by
*        the bootloader to identify that the Applciation started
*        successfully.
*/
void initApplicationRecord(void) {
    msp430Flash_erase_segment(APR_LOCATION);
    appRecord_t *aprP = (appRecord_t *)APR_LOCATION;
    uint16_t temp;
    temp = APR_MAGIC1;
    msp430Flash_write_bytes((uint8_t *)&aprP->magic1, (uint8_t *)&temp, sizeof(uint16_t));
    temp = APR_MAGIC2;
    msp430Flash_write_bytes((uint8_t *)&aprP->magic2, (uint8_t *)&temp, sizeof(uint16_t));
    temp = gen_crc16(APR_LOCATION, (sizeof(appRecord_t) - sizeof(uint16_t)));
    msp430Flash_write_bytes((uint8_t *)&aprP->crc16, (uint8_t *)&temp, sizeof(uint16_t));
}

/**
* \brief Determine if a valid Application Record is located in 
*        the INFO C section.  The record is written by the
*        Application and used by the bootloader to identify that
*        the Applciation started successfully.
* 
* @return bool Returns true if the record is valid.  False 
*         otherwise.
*/
bool checkForApplicationRecord(void) {
    bool aprFlag = false;
    appRecord_t *aprP = (appRecord_t *)APR_LOCATION;
    if ((aprP->magic1 == APR_MAGIC1) && (aprP->magic2 == APR_MAGIC2)) {
        unsigned int crc16 = gen_crc16(APR_LOCATION, (sizeof(appRecord_t) - sizeof(uint16_t)));
        if (crc16 == aprP->crc16) {
            aprFlag = true;
        }
    }
    return aprFlag;
}
