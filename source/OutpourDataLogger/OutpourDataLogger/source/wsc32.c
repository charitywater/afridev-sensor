//
//  wsc32.c    VERSION 4.2.1
//
//  Windows Standard Serial Communications Library
//  Copyright (C) MarshallSoft Computing, Inc. 1997-2006.
//
//  2.0.0 01/28/97 Original version
//  2.0.1 03/09/97 wsc.h & makefile changed.
//                 SioParms & SioFlow return 0 if OK.
//                 SioDTR & SioRTS return WSC_RANGE if bad parameter
//  2.1.0 05/07/97 Added "Expires" option to SioInfo.
//        05/17/97 Added SioRead.
//  2.1.1 06/03/97 Added args to SioWinErr to return error text.
//  2.1.2 08/18/97 Increased to 16 ports.
//  2.2.0 08/26/97 Added dllimport & dllexport to wsc.h
//        09/06/97 Changed XonLim and XoffLim values.
//  2.2.1 03/03/98 First arg of SioRead() changed to UART base address.
//  2.2.2 04/08/98 DTR & RTS flow control disabled at startup.
//  2.3.0 06/24/98 Only NBR_PORTS must be modified to change # of ports.
//        06/29/98 Added SioTimer() function.
//        06/29/98 SioBaud & SioParms can be called before SioReset.
//        07/12/98 SioRead uses default port address for COM1 thru COM4.
//  2.3.1 07/25/98 Changed WSC_VERSION to 3 hex digits.
//  2.3.2 01/08/99 Increased NBR_PORTS to 20.
//  2.3.3 01/17/99 Made ComText an array.
//  2.3.4 01/26/99 Fixed detection of BREAK in SioBrkSig.
//  2.4.0 03/17/99 Added SioEvent function (WIN32 only).
//        03/17/99 Added SioReset(-1, DTR_Default, RTS_Default).
//  2.4.1 08/13/99 SioEvent returns 1 on success.
//  2.4.2 09/22/99 Ran PC-Lint against source. Source modified.
//  2.4.3 09/28/99 Let # ports to be set from the compile line.
//  2.4.4 01/25/00 Check return code from SetupComm().
//  3.0.0 07/10/00 All constants (wsc.h1 & wsc.h2) begin with "WSC_".
//                 Added SioMessage, which uses win32 thread to send message.
//                 NBR_PORTS set to 32.
//  3.0.1 07/24/00 SioClose & SioMessage terminates running thread, if any.
//  3.1.0 10/25/00 Use thread to allow SioPutc and SioPuts to return immediately.
//  3.1.1 11/29/00 SioDebug('X') prevents calling of RESETDEV.
//  3.1.2 02/23/01 SioDebug returns argument (Parm) if recognized.
//  3.1.3 04/03/01 Modified so can be compiled for Windows CE [USE_WIN_CE].
//                 Default for RESETDEV is "not called". SioDebug('R') to enable.
//                 SioGetc & SioGets zero unused bits (DataBits 5,6,7).
//                 Corrected problem with SioBaud(-1, BaudRateCode).
//  3.1.4 04/27/01 SioDebug returns -1 if no match.
//  3.1.5 05/03/01 Added SioDebug('W') toggle.
//  3.1.6 05/25/01 Added code to detect active threads & to close thread handles.
//  3.1.7 02/07/02 Added USE_THREADS, so can compile version of WSC32.C without threads.
//                 Added SetThreadPriority() [must uncomment]
//  3.1.8 03/26/02 Comm handle not saved in SioReset unless good.
//  3.1.9 03/29/02 SioEvent returns mask that caused event.
//  3.1.10 5/02/02 Added anti-crack code to SioReset.
//  3.2.0 07/15/02 Replaced WSC_VERSION with 'VerAndBld' string.
//                 Remapped Windows message "The system cannot find the file specified" to
//                 "The system cannot open the port specified".
//  3.2.1 02/26/03 Replaced "(LPSTR)" with "(LPSTR") in above windows message (for WIN/CE only)
//  3.2.2 04/18/03 Port added to PostMessage in EventThread().
//  3.3.0 05/14/03 EventThread renamed to MsgThread.
//                 EventHandle renamed to MsgHandle.
//                 Added SioSetInteger()
//                 Moved 'W' flag to SioSetInteger.
//        07/18/03 SioPutc/SioPuts will not block if SioEvent was called.
//                 USE_WIN_CE removed (see WSC4eVC and WSCe4VB)
//  3.3.1 08/28/03 Added SioSetInteger(Port, 'H', 0)  {returns COM handle}
//        09/12/03 Added SioKeyCode and SioGetReg
//  4.0.0 11/11/03 Added support for VC.Net (DLL not affected)
//  4.0.1 12/03/03 If Windows NT/2000/XP, then kill & re-start EventThread in SioGetc & SioGets
//  4.0.2  1/08/04 Fixed problem with SioTxClear().
//         1/14/04 Change is TrialRun code [SW version only].
//  4.0.3  7/16/04 MAJOR: Added overlapped I/O (for non-Win95) so can signal threads to exit w/o killing them.
//                 Increased default burst size to 256
//  4.0.4  7/29/04 SioFlow returns WSC_RANGE if cannot recognize parameter.
//  4.1.0  8/02/04 Adjusted thread priorities.
//  4.1.1  9/03/04 Call SioSetInteger(Port,'X',1) to ignore return code from SetupComm (problem with some USB/serial drivers)
//  4.1.2 12/01/04 SioFlow returns 1 if OK.
//  4.1.3  1/06/05 SioSetInteger(Port, 'S', 1) always forces SioEvent to unblock.
//  4.1.4  2/21/05 Event mutex code added to EventThread() to prevent race conditions.
//         3/11/05 Message box displays error if SioWinError(Buffer, 0) called.
//  4.1.5  4/06/05 Modifications to trialDisplayNagScreenTest (TrialRun.inc)
//  4.1.6  4/27/05 Major change in overlapped I/O
//  4.1.7  8/03/05 Removed "if(Size==0) if(Code==WSC_IO_PENDING) return WSC_BUSY" in SioPuts
//  4.1.8  1/12/06 Fixed problem: SioEvent returning wrong code.
//  4.2.0  1/18/06 SioRxClear clears byte saved by SioUnGet
//                 NBR_PORTS increased to maximum of 256.
//                 Added SioEventChar() and SioEventWait() functions.
//  4.2.1  3/03/06 Fixed problem with SioTxQue
//
// ****** IN TEST ************
///#define DEBUG_THREAD_STATUS
// ***************************
 
#define NBR_PORTS  256
 
#define DLL_SOURCE_CODE
#define WIN32_LEAN_AND_MEAN
 
#ifndef STRICT
#define STRICT
#endif
 
#ifndef WIN32
#define WIN32
#endif
 
#pragma hdrstop
 
#include <windows.h>
#include <string.h>
#include <time.h>
#include <assert.h>
#include "wsc.h"
 
#ifndef USHORT
  #define USHORT unsigned short
#endif
 
#ifndef ULONG
  #define ULONG unsigned long
#endif
 
#ifndef UINT
  #define UINT unsigned int
#endif
 
#ifdef LCC_CONSTANTS
  #ifndef RESETDEV
    #define RESETDEV  7
  #endif
  #ifndef MAXDWORD
    #define MAXDWORD  0xffffffff
  #endif
  #ifndef LANG_SYSTEM_DEFAULT
    #define LANG_SYSTEM_DEFAULT 2048
  #endif
#endif
 
#define IHV INVALID_HANDLE_VALUE
 
#define POLY  0x1021
 
#define DtrMask 0x01
#define RtsMask 0x02
 
#ifndef NO_SIOREAD
  #ifdef _MSC_VER
    #define USE_WIN_API 0
    #define READ_PORT ReadThePort
  #endif
 
  #ifdef __BORLANDC__
    #define USE_WIN_API 0
    #define READ_PORT ReadThePort
  #endif
 
  #ifdef __WATCOMC__
    #define USE_WIN_API 0
    unsigned int inp(int);
    #define READ_PORT(P) inp((P))
  #endif
#endif
 
 
static volatile int Initialized = 0;
static LPSTR VerAndBld = "$VER 0421 BLD 0001 MAR06 WSC$";  // <-- values are hex.
 
static COMMTIMEOUTS NewTimeouts;
static USHORT UartAddr[4] = {0x03f8, 0x02f8, 0x03e8, 0x02e8};
static DCB PortDCB[NBR_PORTS];    // initialized by VerifyPort()
 
static struct
{volatile HANDLE ComHandle;
 volatile HANDLE MsgHandle;
 //volatile HANDLE EventSignal;
 //volatile HANDLE EventMutex;      // acquire within Sio functions only
 //volatile HANDLE WaitCommSignal;  // used to kill background event initialed by WaitCommEvent
 volatile BYTE   WaitCommFlag;      // set TRUE if WaitCommEvent() was cancelled (by setting WaitCommSignal)
 volatile BYTE   Status;
 volatile BYTE   NextChar;
#ifdef DEBUG_THREAD_STATUS
 volatile BYTE   EventThreadStatus;
#endif
 DCB    *DCBptr;
 COMMTIMEOUTS OldTimeouts;
 HWND   hMsgWnd;
 DWORD  Mask;
 UINT   Port;
 WORD   MsgCode;
 int    PutStatus;
 BYTE   DataMask;
 BYTE   NoWaitFlag;
 int    WaitComm;
} PortData[NBR_PORTS];
 
static COMSTAT ComStat;
static char ComText[] = "\\\\.\\COM1\0\0\0";
 
 
static unsigned BaudValue[10] =
  {110,300,1200,2400,4800,9600,19200,38400,57600,115200};
 
static unsigned BaudRateDefault = 19200;
static BYTE ParityDefault = WSC_NoParity;
static BYTE StopBitsDefault = WSC_OneStopBit;
static BYTE DataBitsDefault = WSC_WordLength8;
static BYTE DTR_Default = DTR_CONTROL_DISABLE;
static BYTE RTS_Default = RTS_CONTROL_DISABLE;
static BYTE ResetDeviceFlag = FALSE;
static BYTE IgnoreSomeReturnCodes = TRUE;
static volatile int EnterMsgThreadCount = 0;
static volatile int ExitMsgThreadCount = 0;
 
static volatile int DebugInteger1 = 0; // used for debugging
static volatile int DebugInteger2 = 0; // used for debugging
static volatile int CannotCloseHandleCount = 0;
static volatile int EnableMsgBox = 1;
 
 
 
///static int  DebugValue;
 
static DWORD LastWinError = 0;
static int   IsWindows95 = FALSE;  // TRUE if Windows 98 or above
 
#ifndef STATIC_LIBRARY
//**************//  DLL  ************************
 
BOOL WINAPI DllEntryPoint( HINSTANCE hinstDll,
   DWORD fdwRreason,
   LPVOID plvReserved)
{
 return 1;
}
 
int FAR PASCAL WEP ( int bSystemExit )
{
 return 1;
}
 
//***********************************************
#endif
 
//*** PRIVATE functions ***
 
#ifndef MSGBOX
static void MsgBox(LPSTR Ptr)
{MessageBox(NULL,Ptr,(LPSTR)"Info",MB_TASKMODAL|MB_ICONEXCLAMATION);
}
#endif
 
#ifdef __WATCOMC__
#else
  static unsigned char ReadThePort(int Port)
 {unsigned char PortByte;
   _asm  mov edx , Port
   _asm  in  al , dx
   _asm  mov PortByte , al
   return PortByte;
  }
#endif
 
static int IsWin95(void)
{DWORD WindowsVersion;
 DWORD MajorVersion;
 DWORD MinorVersion;
 WindowsVersion = GetVersion();
 MajorVersion =  (DWORD) (LOBYTE(LOWORD(WindowsVersion)));
 MinorVersion =  (DWORD) (HIBYTE(LOWORD(WindowsVersion)));
 if((MajorVersion==4)&&(MinorVersion==0)) return TRUE;
 else return FALSE;
}
 
//-------------[ Overlapped I/O functions ]-------------
 
#define WSC_IO_READY     0
 
typedef struct
{HANDLE   hComm;            // serial port handle
 volatile OVERLAPPED OverlappedRead;   // OIO READ data
 volatile OVERLAPPED OverlappedWrite;  // OIO WRITE data
 volatile OVERLAPPED OverlappedEvent;  // OIO EVENT data
 volatile int        LastReadResult;   // last OIO READ result
 volatile int        LastWriteResult;  // last OIO WRITE result
 volatile int        LastEventResult;  // last OIO EVENT result
 volatile DWORD      BytesRead;
 volatile DWORD      BytesWritten;
} oioStuffType;
 
static oioStuffType oioStuff[NBR_PORTS]; // index by port (COM1, COM2, ...)
static oioIsWindows95 = FALSE;           // set to TRUE if machine is Win95
 
//***********************************************************************
//*** NOTE: Pass NULL for oioPtr to do non-overlapped serial port I/O ***
//***       Win95 does NOT support (serial port) overlapped I/O       ***
//***       Win98 thru WinXP supports (serial port) overlapped I/O    ***
//***********************************************************************
 
//--- __oioCreateEvent() called from oioInit() ONLY ---
 
static HANDLE __oioCreateEvent(LPOVERLAPPED oioPtr)
{// create MANUAL RESET event for overlapped I/O.
 oioPtr->hEvent = CreateEvent(
                        NULL,   // default security attributes
                        TRUE,   // manual reset event
                        FALSE,  // not signaled
                        NULL);  // no name
 // intialize the rest of the OVERLAPPED structure to zero.
 oioPtr->Internal = 0;
 oioPtr->InternalHigh = 0;
 oioPtr->Offset = 0;
 oioPtr->OffsetHigh = 0;
 assert(oioPtr->hEvent);
 return oioPtr->hEvent;
}
 
// signal event
 
static int oioEventSignal(int Port)
{LPOVERLAPPED oioPtr;
 if(oioIsWindows95) return 1;
 oioPtr = (LPOVERLAPPED)&oioStuff[Port].OverlappedEvent;
 if(oioPtr) SetEvent(oioPtr->hEvent);
 return 1;
}
 
// initial oio for one port
 
static int oioInit(int Port, HANDLE hComm, int IsWin95)
{int Code;
 LPOVERLAPPED oioPtr;
 // initialize counts
 oioStuff[Port].BytesRead = 0;
 oioStuff[Port].BytesWritten = 0;
 // save hComm
 oioStuff[Port].hComm = hComm;
 if(IsWin95)
   {// can't do overlapped I/O in Windows 95
    oioIsWindows95 = TRUE;
    return WSC_IO_COMPLETE;
   }
 // create READ signal
 oioPtr = (LPOVERLAPPED)&oioStuff[Port].OverlappedRead;
 Code = (int)__oioCreateEvent(oioPtr);
 if(Code==0) return WSC_IO_ERROR;
 // create WRITE signal
 oioPtr = (LPOVERLAPPED)&oioStuff[Port].OverlappedWrite;
 Code = (int)__oioCreateEvent(oioPtr);
 if(Code==0) return WSC_IO_ERROR;
 // create EVENT signal
 oioPtr = (LPOVERLAPPED)&oioStuff[Port].OverlappedEvent;
 Code = (int)__oioCreateEvent(oioPtr);
 if(Code==0) return WSC_IO_ERROR;
 // initial "last result"
 oioStuff[Port].LastReadResult  = WSC_IO_READY;
 oioStuff[Port].LastWriteResult = WSC_IO_READY;
 oioStuff[Port].LastEventResult = WSC_IO_READY;
 return WSC_IO_COMPLETE;
}
 
static int oioDone(int Port)
{LPOVERLAPPED oioPtr;
 if(oioIsWindows95) return WSC_IO_COMPLETE;
 // close Overlappped READ signal
 oioPtr = (LPOVERLAPPED)&oioStuff[Port].OverlappedRead;
 CloseHandle(oioPtr->hEvent);
 oioPtr->hEvent = 0;
 // close Overlappped WRITE signal
 oioPtr = (LPOVERLAPPED)&oioStuff[Port].OverlappedWrite;
 CloseHandle(oioPtr->hEvent);
 oioPtr->hEvent = 0;
 // close Overlappped EVENT signal
 oioPtr = (LPOVERLAPPED)&oioStuff[Port].OverlappedEvent;
 CloseHandle(oioPtr->hEvent);
 oioPtr->hEvent = 0;
 return WSC_IO_COMPLETE;
}
 
// start overlapped event
 
static int oioEventStart(int Port, DWORD *MaskPtr)
{LPOVERLAPPED oioPtr;
 HANDLE hComm;
 if(oioIsWindows95) return WSC_IO_COMPLETE;
 oioPtr = (LPOVERLAPPED)&oioStuff[Port].OverlappedEvent;
 hComm = oioStuff[Port].hComm;
 // set mask
 if(!SetCommMask(hComm, *MaskPtr))
   {oioStuff[Port].LastEventResult = WSC_IO_ERROR;
    return WSC_IO_ERROR;
   }
 if(oioPtr!=NULL) ResetEvent(oioPtr->hEvent);
 // setup wait comm event
 if(WaitCommEvent(hComm, MaskPtr, oioPtr))
   {// event has ocurred
    oioStuff[Port].LastEventResult = WSC_IO_COMPLETE;
    return WSC_IO_COMPLETE;
   }
 else
   {// error or event is pending
    DWORD LastWinError = GetLastError();
    if(LastWinError==ERROR_IO_PENDING)
      {oioStuff[Port].LastEventResult = WSC_IO_PENDING;
       ///MsgBox((char *)"WSC_IO_PENDING"); ////////////////
       return WSC_IO_PENDING;
      }
    else
      {oioStuff[Port].LastEventResult = WSC_IO_ERROR;
       ///MsgBox((char *)"WSC_IO_ERROR"); ////////////////
       return WSC_IO_ERROR;
      }
   }
}
 
// start overlapped read
// -- returns with whatever data is available; does not wait for 'BufSize' bytes
 
static int oioReadStart(int Port, LPSTR Buffer, DWORD BufSize)
{int Code;
 int WinError;
 ///DWORD Errors;
 ///COMSTAT ComStat;
 LPOVERLAPPED oioPtr;
 HANDLE hComm;
 if(oioIsWindows95) oioPtr = NULL;
 else
   {oioPtr = (LPOVERLAPPED)&oioStuff[Port].OverlappedRead;
    if(oioStuff[Port].LastReadResult==WSC_IO_PENDING) return WSC_BUSY;
   }
 hComm = oioStuff[Port].hComm;
 ///ClearCommError(hComm, &Errors, &ComStat);
 *Buffer = '\0';
 oioStuff[Port].BytesRead = 0;
 Code = ReadFile((HANDLE)hComm,
                 (LPVOID)Buffer,
                 (DWORD)BufSize,
                 (LPDWORD)&oioStuff[Port].BytesRead,
                 (LPOVERLAPPED)oioPtr);
 if(Code)
   {oioStuff[Port].LastReadResult = WSC_IO_COMPLETE;
    return WSC_IO_COMPLETE;
   }
 // ReadFile failure ?
 WinError = GetLastError();
 if(WinError==ERROR_IO_PENDING)
   {///oioStuff[Port].BytesRead = 0;
    oioStuff[Port].LastReadResult = WSC_IO_PENDING;
    return WSC_IO_PENDING;
   }
 else
   {oioStuff[Port].LastReadResult = WSC_IO_ERROR;
    return WSC_IO_ERROR;
   }
}
 
// start overlapped write
 
static int oioWriteStart(int Port, LPSTR Buffer, DWORD BufSize)
{int Code;
 ///DWORD Errors;
 ///COMSTAT ComStat;
 int WinError;
 LPOVERLAPPED oioPtr;
 HANDLE hComm;
 if(oioIsWindows95) oioPtr = NULL;
 else
   {if(oioStuff[Port].LastWriteResult==WSC_IO_PENDING) return WSC_BUSY;
    oioPtr = (LPOVERLAPPED)&oioStuff[Port].OverlappedWrite;
   }
  
 hComm = oioStuff[Port].hComm;
 ///ClearCommError(hComm, &Errors, &ComStat);
 oioStuff[Port].BytesWritten = 0;
 Code = WriteFile((HANDLE)hComm,
                  (LPVOID)Buffer,
                  (DWORD)BufSize,
                  (LPDWORD)&oioStuff[Port].BytesWritten,
                  (LPOVERLAPPED)oioPtr);
 if(Code)
   {oioStuff[Port].LastWriteResult = WSC_IO_COMPLETE;
    return WSC_IO_COMPLETE;
   }
 // ReadFile failure ?
 WinError = GetLastError();
 if(WinError==ERROR_IO_PENDING)
   {///oioStuff[Port].BytesWritten = 0;
    oioStuff[Port].LastWriteResult = WSC_IO_PENDING;
    return WSC_IO_PENDING;
   }
 else
   {oioStuff[Port].LastWriteResult = WSC_IO_ERROR;
    return WSC_IO_ERROR;
   }
}
 
// check overlapped READ status
 
static int oioReadWait(int Port, DWORD Timeout)
{int Code;
 LPOVERLAPPED oioPtr;
 if(oioIsWindows95) return WSC_IO_COMPLETE;
 if(oioStuff[Port].LastReadResult == WSC_IO_COMPLETE) return WSC_IO_COMPLETE;
 oioPtr = (LPOVERLAPPED)&oioStuff[Port].OverlappedRead;
 if(oioPtr==NULL) return WSC_IO_COMPLETE;
 Code = WaitForSingleObject(oioPtr->hEvent, Timeout);
 if(Code==WAIT_OBJECT_0)
   {oioStuff[Port].LastReadResult = WSC_IO_COMPLETE;
    return WSC_IO_COMPLETE;
   }
 if(Code==WAIT_TIMEOUT)
   {oioStuff[Port].LastReadResult = WSC_IO_PENDING;
    return WSC_IO_PENDING;
   }
 else
   {oioStuff[Port].LastReadResult = WSC_IO_ERROR;
    return WSC_IO_ERROR;
   }
}
 
// check overlapped WRITE status
 
static int oioWriteWait(int Port, DWORD Timeout)
{int Code;
 LPOVERLAPPED oioPtr;
 if(oioIsWindows95) return WSC_IO_COMPLETE;
 if(oioStuff[Port].LastWriteResult == WSC_IO_COMPLETE) return WSC_IO_COMPLETE;
 oioPtr = (LPOVERLAPPED)&oioStuff[Port].OverlappedWrite;
 if(oioPtr==NULL) return WSC_IO_COMPLETE;
 Code = WaitForSingleObject(oioPtr->hEvent, Timeout);
 if(Code==WAIT_OBJECT_0)
   {oioStuff[Port].LastWriteResult = WSC_IO_COMPLETE;
    return WSC_IO_COMPLETE;
   }
 if(Code==WAIT_TIMEOUT)
   {oioStuff[Port].LastWriteResult = WSC_IO_PENDING;
    return WSC_IO_PENDING;
   }
 else
   {oioStuff[Port].LastWriteResult = WSC_IO_ERROR;
    return WSC_IO_ERROR;
   }
}
 
// check overlapped EVENT status
 
static int oioEventWait(int Port, DWORD Timeout)
{int Code;
 LPOVERLAPPED oioPtr;
 if(oioIsWindows95) return WSC_IO_COMPLETE;
 oioPtr = (LPOVERLAPPED)&oioStuff[Port].OverlappedEvent;
 if(oioPtr==NULL) return WSC_IO_COMPLETE;
 Code = WaitForSingleObject(oioPtr->hEvent, Timeout);
 if(Code==WAIT_OBJECT_0)
   {oioStuff[Port].LastEventResult = WSC_IO_COMPLETE;
    return WSC_IO_COMPLETE;
   }
 if(Code==WAIT_TIMEOUT)
   {oioStuff[Port].LastEventResult = WSC_IO_PENDING;
    return WSC_IO_PENDING;
   }
 else
   {oioStuff[Port].LastEventResult = WSC_IO_ERROR;
    return WSC_IO_ERROR;
   }
}
 
#if 0
static int oioReadStatus(int Port)
{
 return oioStuff[Port].LastReadResult;
}
#endif
 
// get last WRITE result
 
static int oioWriteStatus(int Port)
{
 return oioStuff[Port].LastWriteResult;
}
 
// get bytes READ
 
DWORD oioGetBytesRead(int Port)
{DWORD BytesTransferred;
 LPOVERLAPPED oioPtr;
 HANDLE hComm;
 if(oioIsWindows95) return oioStuff[Port].BytesRead;
 if(oioStuff[Port].LastReadResult==WSC_IO_COMPLETE)
   {
    if(oioStuff[Port].BytesRead>0) return oioStuff[Port].BytesRead;
   }
 oioPtr = (LPOVERLAPPED)&oioStuff[Port].OverlappedRead;
 hComm = oioStuff[Port].hComm;
 // NOTE: call GetOverlappedResult ONLY after getting WSC_IO_PENDING
 if(GetOverlappedResult(hComm, oioPtr, (LPDWORD)&BytesTransferred, TRUE))
   {if(BytesTransferred>(DWORD)0)
      {oioStuff[Port].BytesRead = BytesTransferred;
       return BytesTransferred;
      }
   }
 return 0;
}
 
// get bytes WRITTEN
 
DWORD oioGetBytesWritten(int Port)
{DWORD BytesTransferred;
 LPOVERLAPPED oioPtr;
 HANDLE hComm;
 if(oioIsWindows95) return oioStuff[Port].BytesRead;
 if(oioStuff[Port].LastWriteResult==WSC_IO_COMPLETE)
   {
    if(oioStuff[Port].BytesWritten>0) return oioStuff[Port].BytesWritten;
   }
 oioPtr = (LPOVERLAPPED)&oioStuff[Port].OverlappedWrite;
 hComm = oioStuff[Port].hComm;
 if(GetOverlappedResult(hComm, oioPtr, (LPDWORD)&BytesTransferred, TRUE))
   {if(BytesTransferred>(DWORD)0)
      {oioStuff[Port].BytesWritten = BytesTransferred;
       return BytesTransferred;
      }
   }
 return 0;
}
 
//-----------------------------------------------------------------------------
 
 
static int VerifyPort(int Port)
{ 
 if((Port<0)||(Port>=NBR_PORTS)) return WSC_IE_BADID;
 else return 0;
}
 
 
int CloseTheHandle(HANDLE Handle)
{int Code;
 Code = CloseHandle(Handle);
 if(!Code) CannotCloseHandleCount++;
 return Code;
}
 
//// Thread functions **
 
void MsgThread(PVOID pvoid)
{int Port;
 HWND  hMsgWnd;
 DWORD Mask;
 WORD  MsgCode;
 int   *PortPtr;
 PortPtr = (int *) pvoid;
 Port = *PortPtr;
 hMsgWnd = PortData[Port].hMsgWnd;
 Mask = PortData[Port].Mask;
 MsgCode = PortData[Port].MsgCode;
 EnterMsgThreadCount++;
 // SioEvent will block this thread until the event occurs
 SioEvent(Port, Mask);
 // event has occured: send message
 if(hMsgWnd) PostMessage(hMsgWnd, MsgCode, (WORD)Port, 0L);
 if(PortData[Port].MsgHandle)
   {// close thread handle
    if(CloseTheHandle(PortData[Port].MsgHandle)) PortData[Port].MsgHandle = 0;
   }
 // exit thread
 ExitMsgThreadCount++;
 ExitThread(0);
}
 
void EventThreadError(char *Message, int ErrCode)
{char xxx[128];
 if(EnableMsgBox)
   {wsprintf((char *)xxx,"Error %d : %s", ErrCode, Message);
    MsgBox((char *)xxx);
   }
}
 
// terminate thread if running
 
static int TerminateTheThread(HANDLE Handle)
{int Code;
 DWORD ThreadStatus;
 // only Win95 code needs to terminate a running thread !
 if(!IsWindows95) return FALSE;
 // get the thread status
 GetExitCodeThread(Handle, &ThreadStatus);
 // is thread still running ?
 if(ThreadStatus==STILL_ACTIVE)
   {// attempt to terminate thread
    Code = TerminateThread(Handle, 0);
    if(!Code)
      {if(EnableMsgBox)
         {char xxx[64];
          wsprintf((char *)xxx,(char *)"Cannot terminate thread %d",(int)Handle);
          MsgBox((char *)xxx);
         }
      }
    return TRUE;
   }
 return FALSE;
}
 
 
static void InitializeIfNeeded(void)
{int i;
 // have we already initialized ?
 if(Initialized==0)
   {Initialized = 1;
    // is this Windows 98 or above ?
    IsWindows95 = IsWin95();
  
    for(i=0;i<NBR_PORTS;i++)
      {PortData[i].ComHandle = IHV;
       PortData[i].Status = 0;
       PortData[i].NextChar = '\0';
       PortData[i].DCBptr = &PortDCB[i];
      }
    }
}
 
//// PUBLIC functions **
 
NoMangle int DLL_IMPORT_EXPORT SioKeyCode(unsigned long KeyCode)
{
 InitializeIfNeeded();
 return 1;
}
 
NoMangle int DLL_IMPORT_EXPORT SioGetReg(LPSTR Buffer, int Size)
{int Len = 0;
 InitializeIfNeeded();
 return Len;
}
 
NoMangle int DLL_IMPORT_EXPORT SioReset(int Port, int InQueue, int OutQueue)
{HANDLE hComm;
 DCB  *DCBptr;
 int  Code;
 DWORD Errors;
 if(Port==-1)
   {// set initialization defaults for DTR & RTS
    DTR_Default = 0x01 & (BYTE) InQueue;
    RTS_Default = 0x01 & (BYTE) OutQueue;
    return 0;
   }
 if((Code=VerifyPort(Port))<0) return Code;
 InitializeIfNeeded();
 wsprintf((LPSTR)ComText,"\\\\.\\COM%d",Port+1);
 
 if(IsWindows95)
    hComm = CreateFile((LPSTR)ComText,
                       GENERIC_READ|GENERIC_WRITE, 0, 0, OPEN_EXISTING,
                       FILE_FLAG_NO_BUFFERING,                           // enable non-overlapped I/O
                      0);
 else
   hComm = CreateFile((LPSTR)ComText,
                      GENERIC_READ|GENERIC_WRITE, 0, 0, OPEN_EXISTING,
                      FILE_FLAG_NO_BUFFERING|FILE_FLAG_OVERLAPPED,      // enable overlapped I/O
                      0);
 
 
  
 if(hComm==INVALID_HANDLE_VALUE)
   {LastWinError = GetLastError();
    return WSC_WIN32ERR;
   }
  
 PortData[Port].ComHandle = hComm;
 oioInit(Port, hComm, IsWindows95);
 
 // get old comm timeouts
 if(!GetCommTimeouts(hComm,&(PortData[Port].OldTimeouts)))
   {LastWinError = GetLastError();
    return WSC_WIN32ERR;
   }
 // get comm state
 DCBptr = PortData[Port].DCBptr;
 DCBptr->DCBlength = sizeof(struct _DCB);
 if(!GetCommState(hComm,DCBptr))
   {LastWinError = GetLastError();
    return WSC_WIN32ERR;
   }
 // set our defaults
 DCBptr->BaudRate = BaudRateDefault;
 DCBptr->ByteSize = DataBitsDefault;
 DCBptr->Parity   = ParityDefault;
 DCBptr->StopBits = StopBitsDefault;
 DCBptr->fBinary = 1;
 
 DCBptr->fNull = 0;
 DCBptr->fOutxDsrFlow = 0;
 DCBptr->fOutxCtsFlow = 0;
 
 if(DTR_Default) DCBptr->fDtrControl = DTR_CONTROL_ENABLE;
 else DCBptr->fDtrControl = DTR_CONTROL_DISABLE;
 if(RTS_Default) DCBptr->fRtsControl = RTS_CONTROL_ENABLE;
 else DCBptr->fRtsControl = RTS_CONTROL_DISABLE;
 DCBptr->fInX = 0;
 DCBptr->fOutX = 0;
 DCBptr->fTXContinueOnXoff = 1;
 DCBptr->fDsrSensitivity = 0;
 DCBptr->fAbortOnError = 0;
 DCBptr->XonChar = 0x11;
 DCBptr->XoffChar = 0x13;
 if(InQueue<=512)
   {DCBptr->XonLim =  (UINT)(InQueue/4);
    DCBptr->XoffLim = (UINT)(InQueue/4);
   }
 else
   {DCBptr->XonLim =  128;
    DCBptr->XoffLim = 128;
   }
 //END$SHAREWARE
 // set the DCB
 if(!SetCommState(hComm,DCBptr))
   {LastWinError = GetLastError();
    return WSC_WIN32ERR;
   }
 // setup I/O buffers
 Code = SetupComm(hComm,InQueue,OutQueue);
 if((!IgnoreSomeReturnCodes)&&(!Code)) return WSC_BUFFERS;
 // reset device ?
 if(ResetDeviceFlag)
   {// RESET the port
    if(!EscapeCommFunction(hComm,RESETDEV))
      {LastWinError = GetLastError();
       return WSC_WIN32ERR;
      }
   }
  
 ClearCommError(hComm,&Errors,&ComStat);
 // set comm event mask
 //if(!SetCommMask(hComm,EV_RXCHAR|EV_BREAK))
 //  {LastWinError = GetLastError();
 //   return WSC_WIN32ERR;
 //  }
 // set new timeouts
 NewTimeouts.ReadIntervalTimeout = MAXDWORD;
 NewTimeouts.ReadTotalTimeoutMultiplier = 0;
 NewTimeouts.ReadTotalTimeoutConstant = 0;
 NewTimeouts.WriteTotalTimeoutMultiplier = 0;
 NewTimeouts.WriteTotalTimeoutConstant = 0;
 if(!SetCommTimeouts(hComm,&NewTimeouts))
   {LastWinError = GetLastError();
    return WSC_WIN32ERR;
   }
 // port is open
 PortData[Port].Port = Port;
 PortData[Port].ComHandle = hComm;
 PortData[Port].DataMask = (1 << DataBitsDefault) - 1;
 PortData[Port].PutStatus = 0;
 // create event mutex
   ///PortData[Port].EventMutex = CreateMutex(0, FALSE, 0);
 return 0;
}
 
NoMangle int DLL_IMPORT_EXPORT SioDone(int Port)
{HANDLE hComm;
 int Code;
 if((Code=VerifyPort(Port))<0) return Code;
 hComm = PortData[Port].ComHandle;
 if(hComm==IHV) return WSC_IE_NOPEN;
 // unblock SioEvent (which will also allow MsgThread to unblock)
 oioEventSignal(Port);
 if(PortData[Port].MsgHandle)
   {// terminate thread if running
    TerminateTheThread(PortData[Port].MsgHandle);
    PortData[Port].MsgHandle = 0;
   }
 // restore old timeouts
 SetCommTimeouts(hComm,&(PortData[Port].OldTimeouts));
 PortData[Port].ComHandle = IHV;
 oioDone(Port);
 return CloseTheHandle(hComm);
}
 
NoMangle int DLL_IMPORT_EXPORT SioBaud(int Port, unsigned BaudRate)
{HANDLE hComm;
 int Code;
 // is BaudRate an index ?
 if(BaudRate<=WSC_Baud115200) BaudRate = BaudValue[BaudRate];
 if(Port==-1)
   {BaudRateDefault = BaudRate;
    return 0;
   }
 if((Code=VerifyPort(Port))<0) return Code;
 hComm = PortData[Port].ComHandle;
 if(hComm==IHV) return WSC_IE_NOPEN;
 PortDCB[Port].BaudRate = BaudRate;
 return SetCommState(hComm,&PortDCB[Port]);
}
 
//-------------------------------------------------------
 
NoMangle int DLL_IMPORT_EXPORT SioPutc(int Port, char Byte)
{HANDLE hComm;
 int Code;
 if((Code=VerifyPort(Port))<0) return Code;
 hComm = PortData[Port].ComHandle;
 if(hComm==IHV) return WSC_IE_NOPEN;
 
 // was there a previous WRITE ?
 
 Code = oioWriteStatus(Port);
 if(Code==WSC_IO_PENDING)
    {// yes, WRITE was previously started
     Code = oioWriteWait(Port, 0);
     if(Code==WSC_IO_PENDING)  return WSC_BUSY;
    }
 
 // start the write
 Code = oioWriteStart(Port,(char *)&Byte, 1);
 if(Code==WSC_IO_ERROR) return Code; ////
 
 // wait for write to complete
 Code = oioWriteWait(Port, INFINITE);
 if(Code==WSC_IO_COMPLETE)
    {// return byte count
     return 1;
    }
 else return WSC_NO_DATA;
}
 
NoMangle int DLL_IMPORT_EXPORT SioPuts(int Port, LPSTR Buffer, unsigned Size)
{int Code;
 HANDLE hComm;
 if((Code=VerifyPort(Port))<0) return Code;
 hComm = PortData[Port].ComHandle;
 if(hComm==IHV) return WSC_IE_NOPEN;
 if((Buffer==NULL)||(Size==0)) return WSC_RANGE;
 
 ///if(Size==0) if(Code==WSC_IO_PENDING)  return WSC_BUSY;
 
 // was there a previous WRITE ?
 
 Code = oioWriteStatus(Port);
 if(Code==WSC_IO_PENDING)
    {// yes, WRITE was previously started
     Code = oioWriteWait(Port, 0);
     if(Code==WSC_IO_PENDING)  return WSC_BUSY;
    }
 
 
 // start the write
 Code = oioWriteStart(Port, Buffer, Size);
    ///if(Code==WSC_BUSY) return WSC_BUSY;
 if(Code==WSC_IO_ERROR) return Code;
 
 if(PortData[Port].NoWaitFlag)
   {// return immediately
    return 1;
   }
 else
   {// wait for SioPuts to complete before returning
    Code = oioWriteWait(Port, INFINITE);
    if(Code==WSC_IO_COMPLETE)
       {// return byte count
        return oioGetBytesWritten(Port);
       }
    else return 0;
   }
 //return 0;
}
 
//-------------------------------------------------------
 
NoMangle int DLL_IMPORT_EXPORT SioGets(int Port,LPSTR Buffer,unsigned Size)
{HANDLE hComm;
 DWORD Errors;
 char  Byte;
 int   Code;
 if((Code=VerifyPort(Port))<0) return Code;
 hComm = PortData[Port].ComHandle;
 if(hComm==IHV) return WSC_IE_NOPEN;
 Byte = PortData[Port].NextChar;
 if(Byte!='\0')
   {PortData[Port].NextChar = '\0';
    return 0x00ff & (int)Byte;
   }
 ClearCommError(hComm,&Errors,&ComStat);
 // start READ
 Code = oioReadStart(Port,Buffer, Size);
 if(Code == WSC_IO_ERROR) return Code;
 // wait for READ to complete
 oioReadWait(Port, INFINITE);
 return oioGetBytesRead(Port);
}
 
NoMangle int DLL_IMPORT_EXPORT SioGetc(int Port)
{HANDLE hComm;
 DWORD Errors;
 long  BytesRead;
 char  Byte;
 int   Code;
 if((Code=VerifyPort(Port))<0) return Code;
 hComm = PortData[Port].ComHandle;
 if(hComm==IHV) return WSC_IE_NOPEN;
 Byte = PortData[Port].NextChar;
 if(Byte!='\0')
   {PortData[Port].NextChar = '\0';
    return 0x00ff & (int)Byte;
   }
 ClearCommError(hComm,&Errors,&ComStat);
 // start READ
 Code = oioReadStart(Port,(char *)&Byte, 1);
 if(Code == WSC_IO_ERROR) return Code;
 // wait for READ to complete
 oioReadWait(Port, INFINITE);
 BytesRead = oioGetBytesRead(Port);
 if(BytesRead<=0) return WSC_NO_DATA;
 // return byte
 return PortData[Port].DataMask & (int)Byte;
}
 
//-------------------------------------------------------
 
NoMangle int DLL_IMPORT_EXPORT SioDTR(int Port,char Cmd)
{HANDLE hComm;
 int  Code;
 if((Code=VerifyPort(Port))<0) return Code;
 hComm = PortData[Port].ComHandle;
 if(hComm==IHV) return WSC_IE_NOPEN;
 switch(Cmd)
   {case 'S':
      PortData[Port].Status |= DtrMask;
      return EscapeCommFunction(hComm,SETDTR);
    case 'C':
      PortData[Port].Status &= (~DtrMask);
      return EscapeCommFunction(hComm,CLRDTR);
    case 'R':
      return DtrMask & PortData[Port].Status;
    default:
      return WSC_RANGE;
   }
}
 
NoMangle int DLL_IMPORT_EXPORT SioRTS(int Port,char Cmd)
{HANDLE hComm;
 int  Code;
 if((Code=VerifyPort(Port))<0) return Code;
 hComm = PortData[Port].ComHandle;
 if(hComm==IHV) return WSC_IE_NOPEN;
 switch(Cmd)
   {case 'S':
      PortData[Port].Status |= RtsMask;
      return EscapeCommFunction(hComm,SETRTS);
    case 'C':
      PortData[Port].Status &= (~RtsMask);
      return EscapeCommFunction(hComm,CLRRTS);
    case 'R':
      return RtsMask & PortData[Port].Status;
    default:
      return WSC_RANGE;
   }
}
 
NoMangle int DLL_IMPORT_EXPORT SioTxClear(int Port)
{HANDLE hComm;
 int  Code;
 if((Code=VerifyPort(Port))<0) return Code;
 hComm = PortData[Port].ComHandle;
 if(hComm==IHV) return WSC_IE_NOPEN;
 PurgeComm(hComm, PURGE_TXCLEAR);
 return 0;
}
 
NoMangle int DLL_IMPORT_EXPORT SioRxClear(int Port)
{HANDLE hComm;
 int  Code;
 if((Code=VerifyPort(Port))<0) return Code;
 hComm = PortData[Port].ComHandle;
 if(hComm==IHV) return WSC_IE_NOPEN;
 PurgeComm(hComm, PURGE_RXCLEAR);
 PortData[Port].NextChar = '\0';
 return 0;
}
 
 
NoMangle int DLL_IMPORT_EXPORT SioTxQue(int Port)
{HANDLE hComm;
 DWORD Errors;
 int   Code;
 int   TxBytes;
 if((Code=VerifyPort(Port))<0) return Code;
 hComm = PortData[Port].ComHandle;
 if(hComm==IHV) return WSC_IE_NOPEN;
 if(ClearCommError(hComm,&Errors,&ComStat)) TxBytes = ComStat.cbOutQue;
 else TxBytes = 0;
 
 if(TxBytes<0) TxBytes = 0;
 
 if(PortData[Port].NoWaitFlag) return TxBytes + (int) oioStuff[Port].BytesWritten;
 else return TxBytes;
}
 
 
NoMangle int DLL_IMPORT_EXPORT SioRxQue(int Port)
{HANDLE hComm;
 DWORD Errors;
 int  Code;
 int RxBytes;
 if((Code=VerifyPort(Port))<0) return Code;
 hComm = PortData[Port].ComHandle;
 if(hComm==IHV) return WSC_IE_NOPEN;
 if(ClearCommError(hComm,&Errors,&ComStat)) RxBytes = ComStat.cbInQue;
 else RxBytes = 0;
 return RxBytes;
}
 
NoMangle int DLL_IMPORT_EXPORT SioStatus(int Port, unsigned Mask)
{HANDLE hComm;
 DWORD Errors = 0;
 int   Code;
 if((Code=VerifyPort(Port))<0) return Code;
 hComm = PortData[Port].ComHandle;
 if(hComm==IHV) return WSC_IE_NOPEN;
 ClearCommError(hComm,&Errors,&ComStat);
 return Mask & Errors;
}
 
NoMangle int DLL_IMPORT_EXPORT SioFlow(int Port,char Cmd)
{HANDLE hComm;
 int  Code;
 DCB  *DCBptr;
 if((Code=VerifyPort(Port))<0) return Code;
 hComm = PortData[Port].ComHandle;
 if(hComm==IHV) return WSC_IE_NOPEN;
 DCBptr = PortData[Port].DCBptr;
 switch(Cmd)
   {case 'H':  // HW flow control
      DCBptr->fOutxCtsFlow = 1;
      DCBptr->fRtsControl = RTS_CONTROL_HANDSHAKE;
      DCBptr->fInX = 0;
      DCBptr->fOutX = 0;
      break;
    case 'S': // SW (XON/XOFF) flow control
      DCBptr->fOutxCtsFlow = 0;
      DCBptr->fRtsControl = RTS_CONTROL_DISABLE;
      DCBptr->fInX = 1;
      DCBptr->fOutX = 1;
      break;
    case 'N':  // no flow control
      DCBptr->fOutxCtsFlow = 0;
      DCBptr->fRtsControl = RTS_CONTROL_DISABLE;
      DCBptr->fInX = 0;
      DCBptr->fOutX = 0;
      break;
    default:
      return WSC_RANGE;
   }
 // set DCB
 if(SetCommState(hComm,DCBptr))
   {// maintain state of DTR & RTS
    if(DtrMask&PortData[Port].Status) EscapeCommFunction(hComm,SETDTR);
    if(RtsMask&PortData[Port].Status) EscapeCommFunction(hComm,SETRTS);
   }
 return 1;
}
 
NoMangle int DLL_IMPORT_EXPORT SioParms(int Port,int Parity,int StopBits,int DataBits)
{HANDLE hComm;
 int Code;
 DCB *DCBptr;
 if(Port==-1)
   {// set defaults
    ParityDefault = (BYTE)Parity;
    StopBitsDefault = (BYTE)StopBits;
    DataBitsDefault = (BYTE)DataBits;
    return 0;
   }
 if((Code=VerifyPort(Port))<0) return Code;
 hComm = PortData[Port].ComHandle;
 if(hComm==IHV) return WSC_IE_NOPEN;
 DCBptr = PortData[Port].DCBptr;
 // set paramaters
 DCBptr->Parity = Parity;
 DCBptr->StopBits = StopBits;
 DCBptr->ByteSize = DataBits;
 PortData[Port].DataMask = (1 << DataBits) - 1;
 if(SetCommState(hComm,DCBptr))
   {// maintain state of DTR & RTS
    if(DtrMask&PortData[Port].Status) EscapeCommFunction(hComm,SETDTR);
    if(RtsMask&PortData[Port].Status) EscapeCommFunction(hComm,SETRTS);
 
   }
 return 0;
}
 
NoMangle int DLL_IMPORT_EXPORT SioCTS(int Port)
{HANDLE hComm;
 int   Code;
 DWORD ModemStatus;
 if((Code=VerifyPort(Port))<0) return Code;
 hComm = PortData[Port].ComHandle;
 if(hComm==IHV) return WSC_IE_NOPEN;
 GetCommModemStatus(hComm,&ModemStatus);
 return MS_CTS_ON & ModemStatus;
}
 
NoMangle int DLL_IMPORT_EXPORT SioDSR(int Port)
{HANDLE hComm;
 DWORD  ModemStatus;
 int    Code;
 if((Code=VerifyPort(Port))<0) return Code;
 hComm = PortData[Port].ComHandle;
 if(hComm==IHV) return WSC_IE_NOPEN;
 GetCommModemStatus(hComm,&ModemStatus);
 return MS_DSR_ON & ModemStatus;
}
 
NoMangle int DLL_IMPORT_EXPORT SioRI(int Port)
{HANDLE hComm;
 DWORD  ModemStatus;
 int    Code;
 if((Code=VerifyPort(Port))<0) return Code;
 hComm = PortData[Port].ComHandle;
 if(hComm==IHV) return WSC_IE_NOPEN;
 GetCommModemStatus(hComm,&ModemStatus);
 return MS_RING_ON & ModemStatus;
}
 
NoMangle int DLL_IMPORT_EXPORT SioDebug(int Parm)
{int i;
 switch(Parm)
  {
   case 'R':
     ResetDeviceFlag = TRUE;
     return Parm;
   case 'W':
     //if(NoWaitFlag) NoWaitFlag = FALSE;
     //else NoWaitFlag = TRUE;
     //return Parm;
     for(i=0;i<NBR_PORTS;i++) PortData[i].NoWaitFlag = (BYTE)Parm;
     // fall through...
   case 'm':  // toggle (on/off) MsgBox calls (and most calls to wsprintf)
     EnableMsgBox = 1 - EnableMsgBox;
     if(EnableMsgBox) MsgBox((char *)"MsgBox enabled");
     return EnableMsgBox;
   default:
     return -1;
  }
}
 
NoMangle int DLL_IMPORT_EXPORT SioSetInteger(int Port, int ParmName, LONG ParmValue)
{int Code;
 if((Code=VerifyPort(Port))<0) return Code;
  
 switch(ParmName)
  {case 'X':
     // ignore any error returned from SetupComm (problem in some USB/serial implementations)
     IgnoreSomeReturnCodes = (BYTE) ParmValue;
     return (int)ParmValue;
   case 'O':
     // force overlapped mode ?
     if(ParmValue) IsWindows95 = FALSE;
     else IsWindows95 = TRUE;
     return (int)ParmValue;
   case 'M': return (int)IsWindows95;
   case 'W':
     PortData[Port].NoWaitFlag = (BYTE)ParmValue;
     return (int)ParmValue;
   case 'B':
     //PortData[Port].BurstSize = ParmValue;
     //return (int)ParmValue;
   case 'H':
     switch(ParmValue)
       {
        //case 2: return (int) PortData[Port].MsgHandle;
        default:
        case 1: return (int) PortData[Port].ComHandle;
       }
  
#ifdef DEBUG_THREAD_STATUS
   case 'T':
     //
     return -1;
#endif
  
   case 'S':
     // unblock SioEvent
     oioEventSignal(Port);
     return 2;
   default:
     return -1;
  }
}
 
 
#ifndef READ_HEX_STRING_IS_DEFINED
static ULONG ReadHexString(LPCSTR Ptr, int Count)
{int  i, n;
 char c;
 ULONG Result = 0;
 for(i=0;i<Count;i++)
   {c = *Ptr++;
    n = 0;
    if((c>='0')&&(c<='9')) n = c - '0';
    if((c>='A')&&(c<='F')) n = 10 + c - 'A';
    if((c>='a')&&(c<='f')) n = 10 + c - 'a';
    Result = (Result<<4) + n;
   }
 return Result;
}
#endif
 
NoMangle int DLL_IMPORT_EXPORT SioInfo(char Parm)
{InitializeIfNeeded();
 switch(Parm)
   {case 'V':   // version
      return ReadHexString((LPSTR)VerAndBld + 5, 4);
    case 'B':   // build
      return ReadHexString((LPSTR)VerAndBld + 14, 4);
    case '3':   // 32 bit ?
      return 1;
    case 'I':   // TX interrupts always on
      return 1;
  
    case 'M': return EnterMsgThreadCount;
    case 'm': return ExitMsgThreadCount;
  
    case 'c': return CannotCloseHandleCount;
  
    case '1': return DebugInteger1;
    case '2': return DebugInteger2;
  
    case '?':
    default:
      return -1;
   }
}
 
NoMangle int DLL_IMPORT_EXPORT SioDCD(int Port)
{HANDLE hComm;
 DWORD  ModemStatus;
 int    Code;
 if((Code=VerifyPort(Port))<0) return Code;
 hComm = PortData[Port].ComHandle;
 if(hComm==IHV) return WSC_IE_NOPEN;
 GetCommModemStatus(hComm,&ModemStatus);
 return MS_RLSD_ON & ModemStatus;
}
 
NoMangle int DLL_IMPORT_EXPORT SioBrkSig(int Port,char Cmd)
{HANDLE hComm;
 DWORD  Errors;
 int    Code;
 if((Code=VerifyPort(Port))<0) return Code;
 hComm = PortData[Port].ComHandle;
 if(hComm==IHV) return WSC_IE_NOPEN;
 switch(Cmd)
   {case 'A':  // ASSERT
      return SetCommBreak(hComm);
    case 'C':  // CANCEL
      return ClearCommBreak(hComm);
    case 'D':  // DETECT
      if(!ClearCommError(hComm,&Errors,&ComStat)) return 0;
      return CE_BREAK & Errors;
    default:
      return WSC_RANGE;
   }
}
 
NoMangle int DLL_IMPORT_EXPORT SioUnGetc(int Port,char Chr)
{HANDLE hComm;
 int Code;
 if((Code=VerifyPort(Port))<0) return Code;
 hComm = PortData[Port].ComHandle;
 if(hComm==IHV) return WSC_IE_NOPEN;
 PortData[Port].NextChar = Chr;
 return 0;
}
 
NoMangle int DLL_IMPORT_EXPORT SioWinError(LPSTR Buffer, int Size)
{char LocalBuffer[128];
 LPSTR BufferPtr;
 int BufferSize;
 // use passed buffer ?
 if(Size>0)
   {BufferPtr = Buffer;
    BufferSize = Size;
   }
 else
   {BufferPtr = (LPSTR)&LocalBuffer[0];
    BufferSize = 128;
   }
 // get error message text
 FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM,
      NULL, LastWinError,
      LANG_SYSTEM_DEFAULT,
      (LPSTR)BufferPtr, BufferSize, NULL);
 // change error message for serial ports
 if(lstrcmp(BufferPtr,(LPSTR)"The system cannot find the file specified.\r\n")==0)
   lstrcpy(BufferPtr,(LPSTR)"The system cannot find the port specified.\r\n");
 // pop up message box if passed size is 0
 if(Size==0) MessageBox(NULL,BufferPtr,(LPSTR)"SioWinError",MB_TASKMODAL|MB_ICONEXCLAMATION);
 return (int) LastWinError;
}
 
NoMangle int DLL_IMPORT_EXPORT SioRead(int Port, int Reg)
{
#ifndef NO_SIOREAD
   HANDLE hComm;
   int Code;
   if((Code=VerifyPort(Port))<0) return Code;
   hComm = PortData[Port].ComHandle;
   if(hComm==IHV) return WSC_IE_NOPEN;
  #if USE_WIN_API
   return -1;
  #else
   if(((unsigned)Port)<=COM4) return READ_PORT(UartAddr[Port]+Reg);
   else return -1;
  #endif
#else
  return -1;
#endif
}
 
NoMangle DWORD DLL_IMPORT_EXPORT SioTimer(void)
{
 return GetTickCount();
}
 
NoMangle int DLL_IMPORT_EXPORT SioEventWait(int Port, DWORD MaskArg, DWORD Timeout)
{HANDLE hComm;
 int Code;
 if((Code=VerifyPort(Port))<0) return Code;
 hComm = PortData[Port].ComHandle;
 if(hComm==IHV) return WSC_IE_NOPEN;
 // save event mask
 PortData[Port].Mask = MaskArg;
 // block on event mask
 Code = oioEventStart(Port, (LPDWORD)&MaskArg);
 if(Code!=WSC_IO_ERROR) Code = oioEventWait(Port, Timeout);
 return Code;
}
 
NoMangle int DLL_IMPORT_EXPORT SioEventChar(int Port, char EvtChar, DWORD Timeout)
{DCB  *DCBptr;
 HANDLE hComm;
 DCBptr = PortData[Port].DCBptr;
 hComm = PortData[Port].ComHandle;
 // get comm state
 DCBptr = PortData[Port].DCBptr;
 DCBptr->DCBlength = sizeof(struct _DCB);
 if(!GetCommState(hComm,DCBptr))
   {LastWinError = GetLastError();
    return WSC_WIN32ERR;
   }
 // set event character
 DCBptr->EvtChar = EvtChar;
 // now set the DCB
 if(!SetCommState(hComm,DCBptr))
   {LastWinError = GetLastError();
    return WSC_WIN32ERR;
   }
 return SioEventWait(Port, EV_RXFLAG, Timeout);
}
 
 
NoMangle int DLL_IMPORT_EXPORT SioEvent(int Port, DWORD MaskArg)
{ 
 return SioEventWait(Port, MaskArg, INFINITE);
}
 
NoMangle int DLL_IMPORT_EXPORT SioMessage(int Port, HWND hMsgWnd, WORD MsgCode, DWORD Mask)
{HANDLE hComm;
 int Code;
 DWORD ThreadID;
 HANDLE ThreadHandle;
 if((Code=VerifyPort(Port))<0) return Code;
 hComm = PortData[Port].ComHandle;
 if(hComm==IHV) return WSC_IE_NOPEN;
 // acquire event mutex
    ///WaitForSingleObject(PortData[Port].EventMutex, INFINITE);
 Sleep(0);
 // save parameters
 PortData[Port].hMsgWnd = hMsgWnd;
 PortData[Port].Mask = Mask;
 ///PortData[Port].Port = Port;
 PortData[Port].MsgCode = MsgCode;
 if((HWND)hMsgWnd==0)
    {// unblock SioEvent
     oioEventSignal(Port);
     return 1;
    }
 // MsgThread already running ?
 Sleep(0); // this sleep is necessary!
 if(PortData[Port].MsgHandle)
   {   ///ReleaseMutex(PortData[Port].EventMutex);
    return 1;
   }
 // start message thread [the thread's stack is freed when it exits but not if the thread is terminated by another thread.]
 ThreadHandle = CreateThread(0,1024,(LPTHREAD_START_ROUTINE)MsgThread,(void *)&PortData[Port].Port,0,&ThreadID);
 if(ThreadHandle==0)
   {if(EnableMsgBox)
      {MsgBox((char *)"Cannot create message thread");
         ///ReleaseMutex(PortData[Port].EventMutex);
       return WSC_THREAD;
      }
   }
 PortData[Port].MsgHandle = ThreadHandle;
 // set thread priority
 SetThreadPriority(ThreadHandle,THREAD_PRIORITY_TIME_CRITICAL-1);
 // release event mutex
    ///ReleaseMutex(PortData[Port].EventMutex);
 return 0;
}
 
#if 0
NoMangle int DLL_IMPORT_EXPORT SioQuiet(int Port, DWORD MilliSecs)
{DWORD TimeMark;
 TimeMark = GetTickCount() + Millisecs;
 while(GetTickCount() < TimeMark)
   {// anything incoming ?
    if(SioRxQue(Port) return FALSE;
    Sleep(10);
   }
 // no incoming within last 'MilliSecs' milliseconds.
 return TRUE;
}
#endif
