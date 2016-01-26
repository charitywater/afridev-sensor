/** 
 * @file dataLogger_timer.cpp
 * \n Source File
 * \n Butter Data Logging Software 
 * 
 *  \brief Methods and data to support an elapsed or countdown timer.
 */
#include "dataLogger_timer.h"

TestTimer& TestSystemTimer(void) {
    static TestTimer testTimer;
    return testTimer;
}

/****************************************************************************** 
* PUBLIC
*******************************************************************************/

void TestTimer::ResetStartTimestamp(void) {
    TestTimer::GetSystemTimestamp(m_StartTimestamp);
}

void TestTimer::GetStartTimestamp(DWORD &rTimestamp) {
    rTimestamp = m_StartTimestamp;
}

DWORD TestTimer::GetElapsedMillisec(void) {
    DWORD msec;
    DWORD current_time;
    TestTimer::GetSystemTimestamp(current_time);
    msec = current_time - m_StartTimestamp;
    return msec;
}

void TestTimer::GetCurrentTimestamp(DWORD &rTimestamp) {
    DWORD current_time;
    TestTimer::GetSystemTimestamp(current_time);
    rTimestamp = current_time;
};

void TestTimer::ResetCountdownTimer(uint32_t milliseconds) {
    m_CountdownIsExpired = false;
    m_CountdownInMillisec = milliseconds;
    ResetStartTimestamp();
}

bool TestTimer::IsCountdownTimerExpired(void) {
    if (m_CountdownIsExpired) {
        return true;
    }
    uint64_t elapsedMillisec = GetElapsedMillisec();
    if (elapsedMillisec >= m_CountdownInMillisec) {
        m_CountdownIsExpired = true;
        return true;
    }
    return false;
}

void TestTimer::SetCountdownTimerToExpired() {
    m_CountdownIsExpired = true;
}

void TestTimer::Test(void) {
    uint64_t ms;
    TestTimer testTimer;
    testTimer.ResetStartTimestamp();

    DWORD startTimestamp;
    testTimer.GetStartTimestamp(startTimestamp);

    testTimer.ResetCountdownTimer(10);
    while (1) {
        Sleep(1);
        ms = testTimer.GetElapsedMillisec();
        if (testTimer.IsCountdownTimerExpired()) {
            testTimer.ResetCountdownTimer(100);
        }
    }

}

void TestTimer::GetSystemTimestamp(DWORD &rTimestamp) {
    rTimestamp = GetTickCount();
}

/****************************************************************************** 
* PRIVATE
*******************************************************************************/

DWORD TestTimer::TimeSpecDiff(DWORD start, DWORD end) {
    DWORD temp;
    temp = end - start;
    return temp;
}

