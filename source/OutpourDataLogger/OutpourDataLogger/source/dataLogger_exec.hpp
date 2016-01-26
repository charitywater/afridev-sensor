/** 
*  @file dataLogger_exec.hpp
*
*/
#pragma once

#include <windows.h>
#include <stdint.h>
#include <time.h>
#include <stdio.h>
#include <string.h>
#include <conio.h>
#include <iostream>
#include <string>
#include <vector>
#include "dataLogger_timer.h"
#include "outpourPacket.h"

class OpLogger {

public:
    OpLogger(void) {
        m_ComportNumber = 1;
        m_ComportIsOpen = false;
        m_readingCount = 0;
        m_AbortFlag = false;
        m_LogEntryNumber = 0;
    }

    ~OpLogger(void) {
    }

    /**
     *  Main routine for OpLogger. Never returns.
     */
    void Exec(void);

    /**
     *  Set the comport number to use
     * @param comport_number - valid comport number (1-256)
     */
    void setComport(int comport_number) {
        m_ComportNumber = comport_number;
    }

    void Abort(void) { m_AbortFlag = true;}

private:

    typedef enum {
        LOOKING_FOR_PACKET_START,
        LOOKING_FOR_MODEM_CMD,
        LOOKING_FOR_PACKET_LENGTH,
        LOOKING_FOR_PACKET_VALUE_ONE,
        LOOKING_FOR_MSG_TYPE,
        LOOKING_FOR_DATA_END,
        LOOKING_FOR_PACKET_END,
    } parseState_t;

    static const int RX_BUF_SIZE = 1024;

    bool captureOnePacket(void);
    void processOutpourMsgPacket(void);
    void packetParseInit(void);
    void printRawPacket(void);
    void printBanner(uint64_t elapsedTime);
    void printDivider(void);

    bool checkForValidModemCmd(uint8_t rxByte);
    bool checkForValidMsgType(uint8_t rxByte);

    int rawData_LogOpenDataFile(void);
    void rawData_LogWriteData(uint64_t elapsedTime);
    void rawData_LogCloseDataFile(void);

    int consoleData_LogOpenDataFile(void);
    void consoleData_LogWriteData(char buffer[]);
    void consoleData_LogCloseDataFile(void);

    void printDataToConsole(void);

    void copyUnitInfoFromPacketVector(void);
    void printUnitInfo(uint64_t elapsedTime);

    void copyStorageDataFromPacketVector(void);
    void printStorageData(uint64_t elapsedTime);
    int storageData_LogOpenDataFile(void);
    void storageData_LogWriteData(uint64_t elapsedTime);
    void storageData_LogCloseDataFile(void);

    void copyPadStatsFromPacketVector(void);
    void printPadStats(uint64_t elapsedTime);
    int padStats_LogOpenDataFile(void);
    void padStats_LogWriteData(uint64_t elapsedTime);
    void padStats_LogCloseDataFile(void);

    bool m_AbortFlag;

    FILE *m_PadStatsLogFileHandle;
    FILE *m_StorageDataLogFileHandle;
    FILE *m_ConsoleDataLogFileHandle;
    FILE *m_RawDataLogFileHandle;

    uint32_t m_LoggingRate;
    uint32_t m_LogEntryNumber;
    TestTimer m_LogTimer;
    uint64_t m_ElapsedTime;

    int m_ComportNumber;
    bool m_ComportIsOpen;
    char m_ConsoleInputBuffer[80];
    char m_ConsoleOutputBuffer[10*1024];
    unsigned char msgRxBuffer[RX_BUF_SIZE];
    int m_readingCount;
    unitInfo_t m_unitInfo;
    sensorStats_t m_sensorStats;
    storageData_t m_storageData;
    parseState_t m_parseState;

    int m_msgRxBufIndex;
    int m_msgRxBytes;
    int m_packetIndex;
    int m_packetDataIndex;
    int m_packetLength;
    int m_packetModemCmd;
    int m_packetOutpourMsgType;
    bool m_packetCaptureReady;
    std::vector<uint8_t> m_packetVector;

    char m_ConsoleBuffer[10*1024];
    int m_ConsoleBufIndex;
    std::vector<std::string> m_ConsoleStringVector;

};

void smsg_find_com_ports(void);
int smsg_find_first_com_port(void);
int smsg_init_port(int comPortNumber);
char smsg_tx(unsigned char *msg_ptr, unsigned short len);
int smsg_get_message(unsigned char *bufP, int bufSize, int timeout, bool dump);
void smsg_rx_flush(void);
int smsg_get_rxque_level(void);


