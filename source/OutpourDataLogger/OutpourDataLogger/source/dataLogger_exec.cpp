/** 
*  @file smsg.cpp
*  Slamball PC test program for firmware development.  This
*  Module contains the main thread of the program.
*
*
*  WARNING:
*  This code is provided for example purpose only.  There is no guarantee for 
*  correct operation in any manner.
*/

#include "dataLogger_exec.hpp"

/**
* \brief 
* 
*/
void OpLogger::Exec(void) {

    bool doExit = false;
    m_ConsoleBufIndex = 0;
    bool firstPacket = true;
    bool doClearScreen = true;

	// system("cls");

    // Open the com port
    if (!m_ComportIsOpen) {
        smsg_init_port(m_ComportNumber);
        m_ComportIsOpen = true;
    }

    packetParseInit();
    m_LogTimer.ResetStartTimestamp();

    if (!doExit) {
        if (rawData_LogOpenDataFile() != 0) {
            doExit = true;
        }
        if (consoleData_LogOpenDataFile() != 0) {
            doExit = true;
        }
        if (padStats_LogOpenDataFile() != 0) {
            doExit = true;
        }
        if (storageData_LogOpenDataFile() != 0) {
            doExit = true;
        }
    }

    // Flush serial rx queue
    smsg_rx_flush();

    while (!doExit) {

        if (m_AbortFlag) {
            break;
        }

        // get response
        m_msgRxBytes = smsg_get_message(&msgRxBuffer[0], (int)128, (int)10000, false);
        if (m_msgRxBytes > 0) {

            m_msgRxBufIndex = 0;

            while (captureOnePacket() || m_packetCaptureReady) {

                if (m_packetCaptureReady) {

                    // if (firstPacket && (m_packetOutpourMsgType != MSG_TYPE_DEBUG_UNIT_INFO)) {
                    //     m_ConsoleStringVector.clear();
                    //     firstPacket = false;
                    //     continue;
                    // }

                    m_ElapsedTime = m_LogTimer.GetElapsedMillisec();
                    rawData_LogWriteData(m_ElapsedTime);

                    switch (m_packetModemCmd) {
                    case M_COMMAND_PING:                 /**< 00=PING */
                    case M_COMMAND_MODEM_INFO:           /**< 01=MODEM INFO */
                    case M_COMMAND_MODEM_STATUS:         /**< 02=MODEM STATUS */
                    case M_COMMAND_MESSAGE_STATUS:       /**< 03=MESSAGE STATUS */
                    case M_COMMAND_SEND_TEST:            /**< 0x20=SEND TEST */
                    case M_COMMAND_SEND_DATA:            /**< 0x40=SEND DATA */
                    case M_COMMAND_GET_INCOMING_PARTIAL: /**< 0x42=GET INCOMING PARTIAL */
                    case M_COMMAND_DELETE_INCOMING:      /**< 0x43=DELETE INCOMING */
                    case M_COMMAND_POWER_OFF:            /**< 0xe0= POWER OFF */
                        printf("Please wait, modem communication is in process\n");
                        printRawPacket();
                        doClearScreen = true;
                        break;
                    case M_COMMAND_SEND_DEBUG_DATA:      /**< 0x50=SEND DEBUG DATA - FOR OUTPUT INTERNAL DEBUG ONLY */
                        processOutpourMsgPacket();
                        break;
                    }

                    if (m_packetOutpourMsgType == MSG_TYPE_DEBUG_PAD_STATS) {
                        if (doClearScreen) {
                            system("cls");
                            doClearScreen = false;
                        }
                        printDataToConsole();
                        m_ConsoleStringVector.clear();
                        smsg_rx_flush();
                    }

                    packetParseInit();
                }
            } // end while
        }
    }


    rawData_LogCloseDataFile();
    consoleData_LogCloseDataFile();
    padStats_LogCloseDataFile();
    storageData_LogCloseDataFile();
    printf("Exit Logger Exec...\n");
    Sleep(1000);
}

void OpLogger::printDataToConsole(void) {
    COORD Cord;
    Cord.X = 0;
    Cord.Y = 0;
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    SetConsoleCursorPosition(hConsole, Cord);
    for (std::vector<std::string>::const_iterator i = m_ConsoleStringVector.begin(); i != m_ConsoleStringVector.end(); ++i) {
        std::cout << *i;
    }
}

void OpLogger::printDivider(void) {
    m_ConsoleBufIndex = sprintf(m_ConsoleBuffer,
                                "---------------------------------------------------------------------------------\n");
    m_ConsoleStringVector.push_back(m_ConsoleBuffer);
    // fprintf(stdout, m_ConsoleBuffer);
    consoleData_LogWriteData(m_ConsoleBuffer);
}

void OpLogger::printBanner(uint64_t elapsedTime) {
    int status = 0;
    SYSTEMTIME st;
    GetLocalTime(&st);
    int index = 0;

    m_ConsoleBufIndex = sprintf(m_ConsoleBuffer, "\n");
    // fprintf(stdout, m_ConsoleBuffer);
    m_ConsoleStringVector.push_back(m_ConsoleBuffer);
    consoleData_LogWriteData(m_ConsoleBuffer);

    m_ConsoleBufIndex = sprintf(m_ConsoleBuffer,
                                "_________________________________________________________________________________\n");
    // fprintf(stdout, m_ConsoleBuffer);
    m_ConsoleStringVector.push_back(m_ConsoleBuffer);
    consoleData_LogWriteData(m_ConsoleBuffer);

    m_ConsoleBufIndex = sprintf(m_ConsoleBuffer,
                                "=================================================================================\n");
    // fprintf(stdout, m_ConsoleBuffer);
    m_ConsoleStringVector.push_back(m_ConsoleBuffer);
    consoleData_LogWriteData(m_ConsoleBuffer);

    m_ConsoleBufIndex = sprintf(m_ConsoleBuffer, "Logger Entry : %08d,  Logger Run Time: %010llu\n", m_LogEntryNumber, elapsedTime / 1000);
    // fprintf(stdout, m_ConsoleBuffer);
    m_ConsoleStringVector.push_back(m_ConsoleBuffer);
    consoleData_LogWriteData(m_ConsoleBuffer);

    m_ConsoleBufIndex = sprintf(m_ConsoleBuffer, "Current Date : %02d/%02d/%04d, Current Time : %02d:%02d:%02d\n", 
                                st.wMonth, st.wDay, st.wYear, st.wHour, st.wMinute, st.wSecond);
    // fprintf(stdout, m_ConsoleBuffer);
    m_ConsoleStringVector.push_back(m_ConsoleBuffer);
    consoleData_LogWriteData(m_ConsoleBuffer);

    m_ConsoleBufIndex = sprintf(m_ConsoleBuffer,
                                "---------------------------------------------------------------------------------\n");
    // fprintf(stdout, m_ConsoleBuffer);
    m_ConsoleStringVector.push_back(m_ConsoleBuffer);
    consoleData_LogWriteData(m_ConsoleBuffer);
}

void OpLogger::printRawPacket(void) {
    std::vector<uint8_t>::const_iterator it;
    std::cout << "RAW PACKET: ";
    for (it = m_packetVector.begin(); it != m_packetVector.end(); ++it) {
        // std::cout << std::hex << std::setw(2) << *it << ' ';
        // std::cout << std::dec << val << ",";
        uint8_t val = *it;
        printf("%02.2X ", val);
    }
    std::cout << std::endl;
}

void OpLogger::copyUnitInfoFromPacketVector(void) {
    uint8_t *stP = (uint8_t *)&m_unitInfo;
    int totalBytes = sizeof(unitInfo_t);
    int vectorBytes = m_packetVector.size();
    for (int i = 0; i < vectorBytes - 8; i++) {
        *stP++ = m_packetVector[i + 8];
    }
}

void OpLogger::copyPadStatsFromPacketVector(void) {
    uint8_t *stP = (uint8_t *)&m_sensorStats;
    for (int i = 0; i < sizeof(sensorStats_t); i++) {
        *stP++ = m_packetVector[i + 8];
    }
}

void OpLogger::copyStorageDataFromPacketVector(void) {
    uint8_t *stP = (uint8_t *)&m_storageData;
    int totalBytes = sizeof(storageData_t);
    int vectorBytes = m_packetVector.size();
    for (int i = 0; i < vectorBytes - 8; i++) {
        *stP++ = m_packetVector[i + 8];
    }
}

void OpLogger::processOutpourMsgPacket(void) {
    switch (m_packetOutpourMsgType) {
    case MSG_TYPE_FA: //  = 0x00,
    case MSG_TYPE_DAILY: //  = 0x01,
    case MSG_TYPE_WEEKLY: //  = 0x02,
    case MSG_TYPE_OTAREPLY: //  = 0x03,
    case MSG_TYPE_RETRYBYTE: //  = 0x04,
    case MSG_TYPE_CHECKIN: //  = 0x05,
    case MSG_TYPE_DEBUG_PAD_STATS: //  = 0x10,
        if (1) {
            copyPadStatsFromPacketVector();
            printPadStats(m_ElapsedTime);
            padStats_LogWriteData(m_ElapsedTime);
        }
        break;
        copyPadStatsFromPacketVector();
    case MSG_TYPE_DEBUG_STORAGE_INFO:  // = 0x11
        if (1) {
            copyStorageDataFromPacketVector();
            printStorageData(m_ElapsedTime);
            storageData_LogWriteData(m_ElapsedTime);
        }
        break;
    case MSG_TYPE_DEBUG_UNIT_INFO:     // = 0x12
        if (1) {
            copyUnitInfoFromPacketVector();
            m_LogEntryNumber++;
            printBanner(m_ElapsedTime);
            printUnitInfo(m_ElapsedTime);
        }
        break;
    }
}

/******************************************************************************** 
* Raw Data Log
********************************************************************************/

void OpLogger::rawData_LogWriteData(uint64_t elapsedTime) {
    if (m_RawDataLogFileHandle != NULL) {
        std::vector<uint8_t>::const_iterator it;
        fprintf(m_RawDataLogFileHandle, "RAW,%d,%llu,", m_LogEntryNumber, elapsedTime);
        for (it = m_packetVector.begin(); it != m_packetVector.end(); ++it) {
            uint8_t val = *it;
            fprintf(m_RawDataLogFileHandle, "%02.2X,", val);
        }
        fprintf(m_RawDataLogFileHandle, "\n");
    }
}

/**
 * \brief Open the data log file
 * 
 */
int OpLogger::rawData_LogOpenDataFile(void) {
    int status = 0;
    std::string name;
    SYSTEMTIME st;
    // Create the logs directory
    if (!CreateDirectory(".\\logs\\raw", NULL)) {
        if (GetLastError() == ERROR_ALREADY_EXISTS) {
            // directory already exists
        } else {
            // creation failed due to some other reason
            status = -1;
        }
    }
    // Attempt to open the file and write header
    if (status == 0) {
        GetLocalTime(&st);
        char buf[1024];
        // Name format is log_yyyy-mm-dd_hhmmss
        sprintf(buf, ".\\logs\\raw\\raw_log_%4d-%02d-%02d_%02d%02d%02d.txt",
                st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);
        m_RawDataLogFileHandle = fopen(buf, "w");
        if (!m_RawDataLogFileHandle) {
            std::cout << "ERROR:  Could not open file for logging: " << buf << std::endl;
            status = -1;
        }
    }

    return status;
}

/**
 * \brief Close the output data file used to save run data
 * 
 */
void OpLogger::rawData_LogCloseDataFile(void) {
    if (m_RawDataLogFileHandle != NULL) {
        fclose(m_RawDataLogFileHandle);
    }
    m_RawDataLogFileHandle = NULL;
}

/******************************************************************************** 
* Console Data Log
********************************************************************************/
void OpLogger::consoleData_LogWriteData(char buffer[]) {
    if (m_ConsoleDataLogFileHandle != NULL) {
        fprintf(m_ConsoleDataLogFileHandle, buffer);
    }
}

/**
 * \brief Open the data log file
 * 
 */
int OpLogger::consoleData_LogOpenDataFile(void) {
    int status = 0;
    std::string name;
    SYSTEMTIME st;
    // Create the logs directory
    if (!CreateDirectory(".\\logs\\console", NULL)) {
        if (GetLastError() == ERROR_ALREADY_EXISTS) {
            // directory already exists
        } else {
            // creation failed due to some other reason
            status = -1;
        }
    }
    // Attempt to open the file and write header
    if (status == 0) {
        GetLocalTime(&st);
        char buf[1024];
        // Name format is log_yyyy-mm-dd_hhmmss
        sprintf(buf, ".\\logs\\console\\console_log_%4d-%02d-%02d_%02d%02d%02d.txt",
                st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);
        m_ConsoleDataLogFileHandle = fopen(buf, "w");
        if (!m_ConsoleDataLogFileHandle) {
            std::cout << "ERROR:  Could not open file for logging: " << buf << std::endl;
            status = -1;
        }
    }

    return status;
}

/**
 * \brief Close the output data file used to save run data
 * 
 */
void OpLogger::consoleData_LogCloseDataFile(void) {
    if (m_ConsoleDataLogFileHandle != NULL) {
        fclose(m_ConsoleDataLogFileHandle);
    }
    m_ConsoleDataLogFileHandle = NULL;
}

/******************************************************************************** 
* Unit Info
********************************************************************************/
/**
* \brief 
* 
* @param elapsedTime 
*/
void OpLogger::printUnitInfo(uint64_t elapsedTime) {
    unitInfo_t *uiP = &m_unitInfo;
    m_ConsoleBufIndex = sprintf(m_ConsoleBuffer,
                                "%10s: %06u, %10s: %02u.%02u\n"             // productId, fwMajor, fwMinor
                                "%10s: %06u, %10s: %06u, %10s: %06u\n"      // GMT hours, min, sec
                                "%10s: %06u, %10s: %06u, %10s: %06u\n",     // GMT day, month, year

                                "prodId",   uiP->productId, "fwVersion", uiP->fwMajor, uiP->fwMinor,
                                "GMT Hour", uiP->GMThour,   "GMT Min",   uiP->GMTminute, "GMT Sec",  uiP->GMTsecond,
                                "GMT Day",  uiP->GMTday,    "GMT Month", uiP->GMTmonth,  "GMT Year", uiP->GMTyear
                               );
    // fprintf(stdout, m_ConsoleBuffer);
    m_ConsoleStringVector.push_back(m_ConsoleBuffer);
    consoleData_LogWriteData(m_ConsoleBuffer);
}

/******************************************************************************** 
* Storage Data
********************************************************************************/
/**
* \brief 
* 
* @param elapsedTime 
*/
void OpLogger::printStorageData(uint64_t elapsedTime) {
    storageData_t *stdP = &m_storageData;

    printDivider();

    m_ConsoleBufIndex = sprintf(m_ConsoleBuffer,
                                "%10s: %06u, %10s: %06u, %10s: %06u\n"                                // storage hours, min, secs
                                "%10s: %06u, %10s: %06u, %10s: %06hu\n",                              // storage day, week daysActivated
                                "ST Hours",  stdP->storageTime_hours,     "ST Min",    stdP->storageTime_minutes,        "ST Sec",     stdP->storageTime_seconds,
                                "ST Day",    stdP->storageTime_dayOfWeek, "ST Week",   stdP->storageTime_week,           "Days Act",   stdP->daysActivated
                               );
    // fprintf(stdout, m_ConsoleBuffer);
    m_ConsoleStringVector.push_back(m_ConsoleBuffer);
    consoleData_LogWriteData(m_ConsoleBuffer);

    printDivider();

    m_ConsoleBufIndex = sprintf(m_ConsoleBuffer,
                                "%10s: %06u, %10s: %06u, %10s: %06u, %10s: %06u\n",                  // RedFlag, RedDays, RedMapDay, RedMapFull
                                "RF T/F",   stdP->redFlagCondition,    "RF Days",   stdP->redFlagDayCount,
                                "MAP Days", stdP->redFlagMapDay,       "MAP Ready",  stdP->redFlagDataFullyPopulated
                               );
    // fprintf(stdout, m_ConsoleBuffer);
    m_ConsoleStringVector.push_back(m_ConsoleBuffer);
    consoleData_LogWriteData(m_ConsoleBuffer);

    printDivider();

    m_ConsoleBufIndex = sprintf(m_ConsoleBuffer,
                                "%10s: %06u, %10s: %06x, %10s: %06x, %10s: %06x\n",                   // Align T/F, AlignSec, AlighMin, AlighHour
                                "Align T/F", stdP->alignStorageFlag,      "Align Hour", stdP->alignBcdHour24,
                                "Align Min",  stdP->alignBcdMinute,        "Align Sec",  stdP->alignBcdSecond
                               );
    // fprintf(stdout, m_ConsoleBuffer);
    m_ConsoleStringVector.push_back(m_ConsoleBuffer);
    consoleData_LogWriteData(m_ConsoleBuffer);

    printDivider();

    m_ConsoleBufIndex = sprintf(m_ConsoleBuffer,
                                "%10s: %06hu, %10s: %08u, %10s: %06hu\n",  // DailyL, HourML, MinuteML
                                "DailyLiter", stdP->dailyLiters, "Hour ML",  stdP->currentHourML, "minute ML", stdP->currentMinuteML
                               );
    // fprintf(stdout, m_ConsoleBuffer);
    m_ConsoleStringVector.push_back(m_ConsoleBuffer);
    consoleData_LogWriteData(m_ConsoleBuffer);

    printDivider();
}
uint16_t redFlagThreshTable[7];    /**< store redFlag compare thresholds */
void OpLogger::storageData_LogWriteData(uint64_t elapsedTime) {
    storageData_t *stdP = &m_storageData;
    if (m_StorageDataLogFileHandle != NULL) {
        fprintf(m_StorageDataLogFileHandle,
                "%u,%llu,"
                "%06hu,%06hu,%06hu,"          // DailyL, HourML, MinuteML
                "%06u,%06u,%06u,"             // storage hours, min, sec
                "%06u,%06u,%06hu,"            // storage day, week, daysActivated
                "%06u,%06u,%06u,%06u,"        // RedFlag, RedDays, RedMapDay, RedMapFull
                "%06u,%06x,%06x,%06x,"        // Align T/F, AlignSec, AlighMin, AlighHour
                "%06u,%06u,%06u,%06u,%06u,%06u,%06u" // redFlagThreshTable
                "\n",

                m_LogEntryNumber,               elapsedTime / 1000,
                stdP->dailyLiters,              stdP->currentHourML,             stdP->currentMinuteML,
                stdP->storageTime_hours,        stdP->storageTime_minutes,       stdP->storageTime_seconds,
                stdP->storageTime_dayOfWeek,    stdP->storageTime_week,          stdP->daysActivated,
                stdP->redFlagCondition,         stdP->redFlagDayCount,
                stdP->redFlagMapDay,            stdP->redFlagDataFullyPopulated,
                stdP->alignStorageFlag,         stdP->alignBcdSecond,
                stdP->alignBcdMinute,
                stdP->alignBcdHour24,
                stdP->redFlagThreshTable[0],
                stdP->redFlagThreshTable[1],
                stdP->redFlagThreshTable[2],
                stdP->redFlagThreshTable[3],
                stdP->redFlagThreshTable[4],
                stdP->redFlagThreshTable[5],
                stdP->redFlagThreshTable[6]
               );
    }
}

/**
 * \brief Open the data log file
 * 
 */
int OpLogger::storageData_LogOpenDataFile(void) {
    int status = 0;
    std::string name;
    SYSTEMTIME st;
    // Create the logs directory
    if (!CreateDirectory(".\\logs\\storage", NULL)) {
        if (GetLastError() == ERROR_ALREADY_EXISTS) {
            // directory already exists
        } else {
            // creation failed due to some other reason
            status = -1;
        }
    }
    // Attempt to open the file and write header
    if (status == 0) {
        GetLocalTime(&st);
        char buf[1024];
        // Name format is log_yyyy-mm-dd_hhmmss
        sprintf(buf, ".\\logs\\storage\\storage_log_%4d-%02d-%02d_%02d%02d%02d.csv",
                st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);
        m_StorageDataLogFileHandle = fopen(buf, "w");
        if (m_StorageDataLogFileHandle) {
            sprintf(buf, "Run Date,%d/%d/%d %02d:%02d:%02d\n", st.wMonth, st.wDay, st.wYear, st.wHour, st.wMinute, st.wSecond);
            fprintf(m_StorageDataLogFileHandle, buf);

            fprintf(m_StorageDataLogFileHandle,
                    "%s,%s,"
                    "%s, %s, %s,"      // minuteML, DailyL, HourL
                    "%s, %s, %s,"      // storage hour, min, sec
                    "%s, %s, %s,"      // storage day, week daysActivated
                    "%s, %s, %s, %s,"  // RedFlag, RedDays, RedMapDay, RedMapFull
                    "%s, %s, %s, %s,"  // Align T/F, AlignSec, AlighMin, AlighHour
                    "%s, %s, %s, %s, %s, %s, %s"
                    "\n",

                    "EntryNumber", "elapsedTime",
                    "DailyL",    "HourML",   "minuteML",
                    "SHour",     "SMin",     "SSec",
                    "SDay",      "SWeek",    "daysAct",
                    "RedFlag",   "RedDays",
                    "RedMap",    "RedFull",
                    "AlignT/F",  "AlignSec",
                    "AlignSec",  "AlignHour",
                    "RedMap0", "RedMap1", "RedMap2", "RedMap3", "RedMap4", "RedMap5", "RedMap6"
                   );

        } else {
            std::cout << "ERROR:  Could not open file for logging: " << buf << std::endl;
            status = -1;
        }
    }

    return status;
}

/**
 * \brief Close the output data file used to save run data
 * 
 */
void OpLogger::storageData_LogCloseDataFile(void) {
    if (m_StorageDataLogFileHandle != NULL) {
        fclose(m_StorageDataLogFileHandle);
    }
    m_StorageDataLogFileHandle = NULL;
}
/******************************************************************************** 
* PAD STATS
********************************************************************************/

void OpLogger::printPadStats(uint64_t elapsedTime) {
    sensorStats_t *stP = &m_sensorStats;

    // Stats
    m_ConsoleBufIndex = sprintf(m_ConsoleBuffer,
                                /// "%10s: %06hu, %10s: %06u, %10s: 0:[%s] 1:[%s] 2:[%s] 3:[%s] 4:[%s] 5:[%s]\n"  // ml,sub,mask,noWater,unknwn,tempC
                                "%10s: %06hu, %10s: %06u, %10s: %s%s%s%s%s%s\n"  // ml,sub,mask
                                "%10s: %06hu, %10s: %06hu, %10s: %06hu\n",       //noWater,unknwn,tempC
                                "Seconds ML", stP->lastMeasFlowRateInMl, "Submerged", stP->numOfSubmergedPads, "Pads", 
                                (stP->submergedPadsBitMask&0x1) ? "0000" :"    ",
                                (stP->submergedPadsBitMask&0x2) ? "1111" :"    ",
                                (stP->submergedPadsBitMask&0x4) ? "2222" :"    ",
                                (stP->submergedPadsBitMask&0x8) ? "3333" :"    ",
                                (stP->submergedPadsBitMask&0x10)? "4444" :"    ",
                                (stP->submergedPadsBitMask&0x20)? "5555" :"    ",
                                // (stP->submergedPadsBitMask&0x1) ? "Y":"N",
                                // (stP->submergedPadsBitMask&0x2) ? "Y":"N",
                                // (stP->submergedPadsBitMask&0x4) ? "Y":"N",
                                // (stP->submergedPadsBitMask&0x8) ? "Y":"N",
                                // (stP->submergedPadsBitMask&0x10)? "Y":"N",
                                // (stP->submergedPadsBitMask&0x20)? "Y":"N",
                                "NoWater", stP->secondsOfNoWater, "Unknowns", stP->unknowns, "TempC", stP->tempCelcius
                               );
    // fprintf(stdout, m_ConsoleBuffer);
    m_ConsoleStringVector.push_back(m_ConsoleBuffer);
    consoleData_LogWriteData(m_ConsoleBuffer);

    printDivider();

    // Pad Counts
    m_ConsoleBufIndex = sprintf(m_ConsoleBuffer,
                                "%10s: %06hu, %10s: %06hu, %10s: %06hu\n"
                                "%10s: %06hu, %10s: %06hu, %10s: %06hu\n", // Counts
                                "PadCount0", stP->padCounts[0], "PadCount1", stP->padCounts[1], "PadCount2", stP->padCounts[2],
                                "PadCount3", stP->padCounts[3], "PadCount4", stP->padCounts[4], "PadCount5", stP->padCounts[5]
                               );
    // fprintf(stdout, m_ConsoleBuffer);
    m_ConsoleStringVector.push_back(m_ConsoleBuffer);
    consoleData_LogWriteData(m_ConsoleBuffer);

    printDivider();

    // Pad Deltas
    m_ConsoleBufIndex = sprintf(m_ConsoleBuffer,
                                "%10s: %06hd, %10s: %06hd, %10s: %06hd\n" 
                                "%10s: %06hd, %10s: %06hd, %10s: %06hd\n", // Delta
                                "PadDelta0", stP->padMeasDelta[0], "PadDelta1", stP->padMeasDelta[1], "PadDelta2", stP->padMeasDelta[2],
                                "PadDelta3", stP->padMeasDelta[3], "PadDelta4", stP->padMeasDelta[4], "PadDelta5", stP->padMeasDelta[5]
                               );
    // fprintf(stdout, m_ConsoleBuffer);
    m_ConsoleStringVector.push_back(m_ConsoleBuffer);
    consoleData_LogWriteData(m_ConsoleBuffer);

    printDivider();

    // Pad Max/Min/Compare
    m_ConsoleBufIndex = sprintf(m_ConsoleBuffer,
                                "%10s: %06hu, %10s: %06hu, %10s: %06hu\n"
                                "%10s: %06hu, %10s: %06hu, %10s: %06hu\n" // Max1Array
                                "%10s: %06hu, %10s: %06hu, %10s: %06hu\n"
                                "%10s: %06hu, %10s: %06hu, %10s: %06hu\n" // Max2Array
                                "%10s: %06hu, %10s: %06hu, %10s: %06hu\n"
                                "%10s: %06hu, %10s: %06hu, %10s: %06hu\n" // Max
                                "%10s: %06hu, %10s: %06hu, %10s: %06hu\n"
                                "%10s: %06hu, %10s: %06hu, %10s: %06hu\n", // Min

                                "PadCmpA0", stP->pad_max1_array[0], "PadCmpA1", stP->pad_max1_array[1], "PadCmpA2", stP->pad_max1_array[2],
                                "PadCmpA3", stP->pad_max1_array[3], "PadCmpA4", stP->pad_max1_array[4], "PadCmpA5", stP->pad_max1_array[5],

                                "PadCmpB0", stP->pad_max2_array[0], "PadCmpB1", stP->pad_max2_array[1], "PadCmpB2", stP->pad_max2_array[2],
                                "PadCmpB3", stP->pad_max2_array[3], "PadCmpB4", stP->pad_max2_array[4], "PadCmpB5", stP->pad_max2_array[5],

                                "PadMax0", stP->pad_max[0], "PadMax1", stP->pad_max[1], "PadMax2", stP->pad_max[2],
                                "PadMax3", stP->pad_max[3], "PadMax4", stP->pad_max[4], "PadMax5", stP->pad_max[5],

                                "PadMin0", stP->pad_min[0], "PacMin1", stP->pad_min[1], "PadMin2", stP->pad_min[2],
                                "PadMin1", stP->pad_min[3], "PadMin2", stP->pad_min[4], "PadMin3", stP->pad_min[5]
                               );
    // fprintf(stdout, m_ConsoleBuffer);
    m_ConsoleStringVector.push_back(m_ConsoleBuffer);
    consoleData_LogWriteData(m_ConsoleBuffer);

}

void OpLogger::padStats_LogWriteData(uint64_t elapsedTime) {
    sensorStats_t *stP = &m_sensorStats;
    if (m_PadStatsLogFileHandle != NULL) {
        fprintf(m_PadStatsLogFileHandle,
                "%u,%llu,"
                "%hu,%u,%x,%hu,%hu,%hu,"
                "%hu,%hu,%hu,%hu,%hu,%hu,"  // padCounts
                "%hu,%hu,%hu,%hu,%hu,%hu,"  // pad_max
                "%hu,%hu,%hu,%hu,%hu,%hu,"  // pad_min
                "%hd,%hd,%hd,%hd,%hd,%hd,"  // padMeasDelta
                "%hu,%hu,%hu,%hu,%hu,%hu,"  // pad_max1_array
                "%hu,%hu,%hu,%hu,%hu,%hu,"  // pad_max2_array
                "\n",
                m_LogEntryNumber, elapsedTime,

                stP->lastMeasFlowRateInMl, stP->numOfSubmergedPads, stP->submergedPadsBitMask,
                stP->secondsOfNoWater, stP->unknowns, stP->tempCelcius,

                stP->padCounts[0], stP->padCounts[1], stP->padCounts[2],
                stP->padCounts[3], stP->padCounts[4], stP->padCounts[5],

                stP->pad_max[0], stP->pad_max[1], stP->pad_max[2],
                stP->pad_max[3], stP->pad_max[4], stP->pad_max[5],

                stP->pad_min[0], stP->pad_min[1], stP->pad_min[2],
                stP->pad_min[3], stP->pad_min[4], stP->pad_min[5],

                stP->padMeasDelta[0], stP->padMeasDelta[1], stP->padMeasDelta[2],
                stP->padMeasDelta[3], stP->padMeasDelta[4], stP->padMeasDelta[5],

                stP->pad_max1_array[0], stP->pad_max1_array[1], stP->pad_max1_array[2],
                stP->pad_max1_array[3], stP->pad_max1_array[4], stP->pad_max1_array[5],

                stP->pad_max2_array[0], stP->pad_max2_array[1], stP->pad_max2_array[2],
                stP->pad_max2_array[3], stP->pad_max2_array[4], stP->pad_max2_array[5]
               );
    }
}

/**
 * \brief Open the data log file
 * 
 */
int OpLogger::padStats_LogOpenDataFile(void) {
    int status = 0;
    std::string name;
    SYSTEMTIME st;
    // Create the logs directory
    if (!CreateDirectory(".\\logs\\padStats", NULL)) {
        if (GetLastError() == ERROR_ALREADY_EXISTS) {
            // directory already exists
        } else {
            // creation failed due to some other reason
            status = -1;
        }
    }
    // Attempt to open the file and write header
    if (status == 0) {
        GetLocalTime(&st);
        char buf[1024];
        // Name format is log_yyyy-mm-dd_hhmmss
        sprintf(buf, ".\\logs\\padStats\\padStats_log_%4d-%02d-%02d_%02d%02d%02d.csv",
                st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);
        m_PadStatsLogFileHandle = fopen(buf, "w");
        if (m_PadStatsLogFileHandle) {
            sprintf(buf, "Run Date,%d/%d/%d %02d:%02d:%02d\n", st.wMonth, st.wDay, st.wYear, st.wHour, st.wMinute, st.wSecond);
            fprintf(m_PadStatsLogFileHandle, buf);

            fprintf(m_PadStatsLogFileHandle,
                    "%s,%s,"
                    "%s,%s,%s,%s,%s,%s,"  // lastMeasFlowRateInMl, numOfSub, sub, sec, unknown, tempCelcius
                    "%s,%s,%s,%s,%s,%s,"  // padCounts
                    "%s,%s,%s,%s,%s,%s,"  // pad_max
                    "%s,%s,%s,%s,%s,%s,"  // pad_min
                    "%s,%s,%s,%s,%s,%s,"  // padMeasDelta
                    "%s,%s,%s,%s,%s,%s,"  // padMax1Array
                    "%s,%s,%s,%s,%s,%s,"  // padMax2Array
                    "\n",

                    "EntryNumber", "elapsedTime",

                    "lastMeasFlowRateInMl", "numOfSubmergedPads", "submergedPadsBitMask",
                    "secondsOfNoWater", "unknowns", "tempC",

                    "PC0", "PC1", "PC2", "PC3", "PC4", "PC5",

                    "PMAX0", "PMAX1", "PMAX2", "PMAX3", "PMAX4", "PMAX5",

                    "pad_min[0]", "pad_min[1]", "pad_min[2]", "pad_min[3]", "pad_min[4]", "pad_min[5]",

                    "padMeasDelta[0]", "padMeasDelta[1]", "padMeasDelta[2]",
                    "padMeasDelta[3]", "padMeasDelta[4]", "padMeasDelta[5]",

                    "padMax1Array[0]", "padMax1Array[1]", "padMax1Array[2]",
                    "padMax1Array[3]", "padMax1Array[4]", "padMax1Array[5]",

                    "padMax2Array[0]", "padMax2Array[1]", "padMax2Array[2]",
                    "padMax2Array[3]", "padMax2Array[4]", "padMax2Array[5]"

                   );

        } else {
            std::cout << "ERROR:  Could not open file for logging: " << buf << std::endl;
            status = -1;
        }
    }

    return status;
}

/**
 * \brief Close the output data file used to save run data
 * 
 */
void OpLogger::padStats_LogCloseDataFile(void) {
    if (m_PadStatsLogFileHandle != NULL) {
        fclose(m_PadStatsLogFileHandle);
    }
    m_PadStatsLogFileHandle = NULL;
}

/**
* \brief Look for the pieces of the packet to detect the start, 
*        verify that it is a packet, and capture until the end.
* 
* @return bool Returns true if a complete packet was captured. 
*/
bool OpLogger::captureOnePacket(void) {
    bool ready = false;
    do {
        switch (m_parseState) {
        case LOOKING_FOR_PACKET_START:
            for (; m_msgRxBufIndex < m_msgRxBytes;) {
                uint8_t rxByte = msgRxBuffer[m_msgRxBufIndex++];
                if (rxByte == 0x3C) {
                    m_packetVector.push_back(rxByte);
                    m_packetIndex++;
                    m_parseState = LOOKING_FOR_MODEM_CMD;
                    break;
                }
            }
            break;
        case LOOKING_FOR_MODEM_CMD:
            // The modem command byte is at offset 1 of the packet
            for (; m_msgRxBufIndex < m_msgRxBytes;) {
                uint8_t rxByte = msgRxBuffer[m_msgRxBufIndex++];
                m_packetVector.push_back(rxByte);
                if ((m_packetIndex++ == 1) && (checkForValidModemCmd(rxByte) == true)) {
                    m_packetModemCmd = rxByte;

                    switch (rxByte) {
                    case M_COMMAND_PING:                 /**< 00=PING */
                    case M_COMMAND_MODEM_INFO:           /**< 01=MODEM INFO */
                    case M_COMMAND_MODEM_STATUS:         /**< 02=MODEM STATUS */
                    case M_COMMAND_MESSAGE_STATUS:       /**< 03=MESSAGE STATUS */
                    case M_COMMAND_SEND_TEST:            /**< 0x20=SEND TEST */
                    case M_COMMAND_GET_INCOMING_PARTIAL: /**< 0x42=GET INCOMING PARTIAL */
                    case M_COMMAND_DELETE_INCOMING:      /**< 0x43=DELETE INCOMING */
                    case M_COMMAND_POWER_OFF:            /**< 0xe0= POWER OFF */
                        m_parseState = LOOKING_FOR_PACKET_END;
                        break;

                    case M_COMMAND_SEND_DATA:            /**< 0x40=SEND DATA */
                    case M_COMMAND_SEND_DEBUG_DATA:      /**< 0x50=SEND DEBUG DATA - FOR OUTPUT INTERNAL DEBUG ONLY */
                        m_parseState = LOOKING_FOR_PACKET_LENGTH;
                        break;
                    }

                    break;
                } else {
                    packetParseInit();
                    break;
                }
            }
            break;
        case LOOKING_FOR_PACKET_LENGTH:
            // The packet length is at offset 5
            for (; m_msgRxBufIndex < m_msgRxBytes;) {
                uint8_t rxByte = msgRxBuffer[m_msgRxBufIndex++];
                m_packetVector.push_back(rxByte);
                if (m_packetIndex++ == 5) {
                    m_packetLength = rxByte;
                    // Account for header, CRC bytes and end byte (h=5,crc=2,end=1)
                    m_packetLength += 8;
                    m_parseState = LOOKING_FOR_PACKET_VALUE_ONE;
                    break;
                }
            }
            break;
        case LOOKING_FOR_PACKET_VALUE_ONE:
            // A constant of 0x1 is at offset 6
            for (; m_msgRxBufIndex < m_msgRxBytes;) {
                uint8_t rxByte = msgRxBuffer[m_msgRxBufIndex++];
                m_packetVector.push_back(rxByte);
                if ((m_packetIndex++ == 6) && (rxByte == 0x1)) {
                    m_parseState = LOOKING_FOR_MSG_TYPE;
                    break;
                } else {
                    packetParseInit();
                    break;
                }
            }
            break;
        case LOOKING_FOR_MSG_TYPE:
            // The outpour message type is at offset 7
            for (; m_msgRxBufIndex < m_msgRxBytes;) {
                uint8_t rxByte = msgRxBuffer[m_msgRxBufIndex++];
                m_packetVector.push_back(rxByte);
                if ((m_packetIndex++ == 7) && (checkForValidMsgType(rxByte) == true)) {
                    m_packetOutpourMsgType = rxByte;
                    m_parseState = LOOKING_FOR_DATA_END;
                    break;
                } else {
                    packetParseInit();
                    break;
                }
            }
            break;
        case LOOKING_FOR_DATA_END:
            // Get remaining portion of packet
            for (; m_msgRxBufIndex < m_msgRxBytes;) {
                uint8_t rxByte = msgRxBuffer[m_msgRxBufIndex++];
                m_packetVector.push_back(rxByte);
                if ((m_packetIndex++ == m_packetLength) && (rxByte == 0x3B)) {
                    ready = true;
                    m_packetCaptureReady = true;
                    break;
                } else if (m_packetIndex > m_packetLength) {
                    packetParseInit();
                    break;
                }
            }
            break;
        case LOOKING_FOR_PACKET_END:
            // Get remaining portion of packet
            for (; m_msgRxBufIndex < m_msgRxBytes;) {
                uint8_t rxByte = msgRxBuffer[m_msgRxBufIndex++];
                m_packetVector.push_back(rxByte);
                if (rxByte == 0x3B) {
                    ready = true;
                    m_packetCaptureReady = true;
                    break;
                }
            }
            break;

        } // end case
    } while ((m_msgRxBufIndex < m_msgRxBytes) && !ready);

    return (m_msgRxBufIndex < m_msgRxBytes);
}

void OpLogger::packetParseInit(void) {
    m_parseState = LOOKING_FOR_PACKET_START;
    m_packetIndex = 0;
    m_packetDataIndex = 0;
    m_packetCaptureReady = false;
    m_packetVector.clear();
}

/*
Example Debug Packet
0x3c 0x50 0x00 0x00 0x00 0x48 0x01 0x10 0x6a 0x01 0x01 0x01 0x00 0x00 0xc2 0x9f 0x8d 0xa9 0x8b 0xa5
0x3d 0xab 0x2a 0xb1 0xf0 0xbf 0xc2 0x9f 0x8d 0xa9 0x9a 0xa5 0x47 0xab 0x2a 0xb1 0xf6 0xbf 0xc0 0x9f
0x88 0xa9 0x8b 0xa5 0x38 0xab 0x1b 0xb1 0xe7 0xbf 0x00 0x03 0x29 0x00 0x2a 0x00 0x22 0x00 0x21 0x00
0x28 0x00 0x03 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x03 0x00 0x00 0x00 0x01 0x40
0x3b
*/

bool OpLogger::checkForValidModemCmd(uint8_t rxByte) {
    bool validCmd = false;
    // printf("\n\n GOT MODEM COMMAND: 0x%x\n\n", rxByte);
    switch (rxByte) {
    case M_COMMAND_PING:                 /**< 00=PING */
    case M_COMMAND_MODEM_INFO:           /**< 01=MODEM INFO */
    case M_COMMAND_MODEM_STATUS:         /**< 02=MODEM STATUS */
    case M_COMMAND_MESSAGE_STATUS:       /**< 03=MESSAGE STATUS */
    case M_COMMAND_SEND_TEST:            /**< 0x20=SEND TEST */
    case M_COMMAND_SEND_DATA:            /**< 0x40=SEND DATA */
    case M_COMMAND_GET_INCOMING_PARTIAL: /**< 0x42=GET INCOMING PARTIAL */
    case M_COMMAND_DELETE_INCOMING:      /**< 0x43=DELETE INCOMING */
    case M_COMMAND_POWER_OFF:            /**< 0xe0= POWER OFF */
    case M_COMMAND_SEND_DEBUG_DATA:      /**< 0x50=SEND DEBUG DATA - FOR OUTPUT INTERNAL DEBUG ONLY */
        validCmd = true;
        break;
    }
    return validCmd;
}

bool OpLogger::checkForValidMsgType(uint8_t rxByte) {
    bool validMsgType = false;
    switch (rxByte) {
    case MSG_TYPE_FA: //  = 0x00,
    case MSG_TYPE_DAILY: //  = 0x01,
    case MSG_TYPE_WEEKLY: //  = 0x02,
    case MSG_TYPE_OTAREPLY: //  = 0x03,
    case MSG_TYPE_RETRYBYTE: //  = 0x04,
    case MSG_TYPE_CHECKIN: //  = 0x05,
    case MSG_TYPE_DEBUG_PAD_STATS: //  = 0x10,
    case MSG_TYPE_DEBUG_STORAGE_INFO: // = 0x11,
    case MSG_TYPE_DEBUG_UNIT_INFO: // = 0x12
        validMsgType = true;
        break;
    }
    return validMsgType;
}

/*******************************************************************************
*
*  Confirm entry
*
*******************************************************************************/
static bool confirm_entry(const char *msg) {
    char input_buf[128];
    printf("\nconfirm %s [yY]: ", msg);
    gets(input_buf);
    if ((strlen(input_buf) != 1) ||
        ((input_buf[0] != 'y') && (input_buf[0] != 'Y'))) {
        return false;
    }
    return true;
}

