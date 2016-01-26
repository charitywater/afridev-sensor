/** 
 * @file dataLogger_timer.h
 * \n Source File
 * \n Butter Data Logging Software 
 * 
 *  \brief Methods and data to support an elapsed or countdown timer.
 */
#ifndef _TEST_TIME_
#define _TEST_TIME_

// SYSTEM INCLUDES
//
#include <Windows.h>
#include <stdint.h>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <time.h>

// PROJECT INCLUDES
//

/**
 * Class TestTimer
 *  
 * Methods and data to support one elapsed OR countdown timer. A
 * start time is maintained in the object.  It is reset to a 
 * system timestamp (using GetTickCount) when either the 
 * ResetStartTimestamp or ResetCountdownTimer is called. Methods 
 * are provided to return info based on the start or current 
 * time. 
 * 
 */
class TestTimer {

public:

    // Constructor
    TestTimer() {
        m_CountdownIsExpired = false;
    }

    ~TestTimer() {
    }

    /**
     * Set the start timestamp to the current time (as provided by 
     * clock_gettime()). 
     */
    void ResetStartTimestamp(void);

    /**
     * The DWORD reference passed in is written with the start
     * time (start time is set when ResetStartTimestamp or 
     * ResetCountdownTimer was called). 
     * 
     * @param DWORD& rTimestamp  
     */
    void GetStartTimestamp(DWORD &rTimestamp);

    /**
     * Return the elapsed time since the ResetStartTimestamp was called.
     * Elapsed time is returned in milliseconds.
     * 
     * @param DWORD& rTimestamp  
     */
    DWORD GetElapsedMillisec(void);

    /**
     * The DWORD reference passed in is written with the current 
     * system timestamp (currently retrieved using clock_gettime()).
     * 
     * @param DWORD& rTimestamp  
     */
    void GetCurrentTimestamp(DWORD &rTimestamp);

    /**
     *  Reset the countdown timer in milliseconds.  Note that this
     *  resets the start time.
     * 
     * @param uint32_t milliseconds  
     */
    void ResetCountdownTimer(uint32_t milliseconds);

    /**
     *
     * Test if the count down timer has expired. If the countdown 
     * timer has expired, it can be restarted using the 
     * ResetCountdownTimer. 
     * 
     * @return bool - true if expired, false otherwise.
     */
    bool IsCountdownTimerExpired(void);

    /**
     * Force the state of the countdown timer to be expired.
     */
    void SetCountdownTimerToExpired(void);

    static void Test(void);

private:

    DWORD    m_StartTimestamp;
    bool     m_CountdownIsExpired;
    uint32_t m_CountdownInMillisec;

    DWORD TimeSpecDiff(DWORD start, DWORD end);

    void GetSystemTimestamp(DWORD &rTimestamp);
};

/**
 * There is one specially allocated TestTimer which contains the 
 * system start time. It can be accessed by other objects in the 
 * system to obtain the system start time.  The object is 
 * created when the TestSystemTimer method is called the first 
 * time. 
 *  
 * The time is set at system start using: 
 *   TestSystemTime().ResetStartTimestamp(); 
 *
 * The elapsed time since the system was started is returned 
 * using: 
 *   TestSystemTime().GetStartTimestamp () 
 *
 * A formated string of the elapsed time since the system was 
 * started is returned using: 
 *   TestSystemTime().GetStartTimestampString (). 
 * 
 * 
 * @return TestTimer& 
 */
TestTimer& TestSystemTimer(void);

#endif // _TEST_TIME_

