/** 
*  @file smsg_com.cpp
*  Slamball PC test program for firmware development.  This
*  module contains the com port support functions for the
*  program.
*
*
*  WARNING:
*  This code is provided for example purpose only.  There is no guarantee for 
*  correct operation in any manner.
*/

/*******************************************************************************
*
*  UART communication support module
*
*  This module uses the Marshall Soft library to implement 
*  PC UART Serial Communication (www.marshallsoft.com).
*
*******************************************************************************/
#include <windows.h>
#include <time.h>
#include <stdio.h>
#include <string.h>
#include <conio.h>

#include "wsc.h"

// Module data structure
typedef struct smsg_s {
    // Serial parameters
    int  Parity;
    int  StopBits;
    int  DataBits;
    int  Port;
    int  Baud;
} smsg_com_t;

// Module private data
static smsg_com_t smsg_com;
static smsg_com_t *smsgComP = &smsg_com;

// Module private prototypes
static int smsg_error_check(int Code);

/*******************************************************************************
*
*  Find COM ports in the system
*
*******************************************************************************/
void smsg_find_com_ports (void) {
	int i;
	int status;

	printf ("\nExisting COM ports in the system:\n");
	for (i=1;i<256;i++) {
        status = SioReset (i,16,16);
		if (status >= 0) {	
			SioDone(i);
            printf ("Port %d\n", i+1);
	    } else if (status == WSC_IE_BADID) {
		} else if (status == WSC_IE_OPEN) {
		}
	}
	printf ("\n");
}

/*******************************************************************************
*
*  Return first COM port found in the system
*
*******************************************************************************/
int smsg_find_first_com_port (void) {
	int i;
	int status;

	for (i=1;i<256;i++) {
        status = SioReset (i,16,16);
		if (status >= 0) {	
			SioDone(i);
            break;
	    } else if (status == WSC_IE_BADID) {
		} else if (status == WSC_IE_OPEN) {
		}
	}
    return i;
}

/*******************************************************************************
*
*  Initialize COM port and module data
*
*******************************************************************************/
int smsg_init_port (int comPortNumber)
{
    int status;

    // Serial parameters
    smsgComP->Parity   = WSC_NoParity;
    smsgComP->StopBits = WSC_OneStopBit;
    smsgComP->DataBits = WSC_WordLength8;
    smsgComP->Port = comPortNumber-1;
    smsgComP->Baud = 9600;

    status = smsg_error_check( SioReset(smsgComP->Port,(100*1024),(100*1024)) );
    status = smsg_error_check( SioBaud(smsgComP->Port,smsgComP->Baud) );
    status = smsg_error_check( SioDTR(smsgComP->Port,'S') );
    status = smsg_error_check( SioRTS(smsgComP->Port,'S') );
    status = smsg_error_check( SioParms(smsgComP->Port,smsgComP->Parity,
                               smsgComP->StopBits,smsgComP->DataBits) );
    status = smsg_error_check( SioBrkSig(smsgComP->Port, WSC_CANCEL_BREAK) );
    status = smsg_error_check( SioFlow(smsgComP->Port, WSC_NO_FLOW_CONTROL) );

    // printf("    Port : COM%1d\n", 1+smsgComP->Port);
    // printf("    Baud : %u\n", smsgComP->Baud);
    // printf("    Flow : None\n");
    // printf("  Parity : %d\n",smsgComP->Parity);
    // printf("StopBits : %d\n",smsgComP->StopBits);
    // printf("DataBits : %d\n",smsgComP->DataBits);

    return 0;
} 

/*******************************************************************************
*
*  Send buffer out the PC COM port.
*
*******************************************************************************/
char smsg_tx (unsigned char *msg_ptr, unsigned short len)
{
    unsigned int numTx;

#if 0
    int i;
	printf ("\tSending: ");
    for (i=0;i<len;i++) {
        printf ("0x%x ", msg_ptr[i]);
	}
	printf ("\tSending: ");
    for (i=0;i<len;i++) {
        printf ("%d ", msg_ptr[i]);
	}
	printf ("\n");
#endif

#if 1
    while (len) {
        numTx = SioPuts(smsgComP->Port, (LPSTR)msg_ptr, len);
        len -= numTx;
    }
#else
    int i;
    char input;
    for (i=0;i<len;i++) {
        printf ("Sending byte %d: 0x%0x\n", i, *msg_ptr);
        SioPutc (smsgComP->Port, *msg_ptr++);
        while (!_kbhit()) {
            Sleep (500);
        }
        input = getchar ();
    }
#endif

    return(0);
}

/*******************************************************************************
*
*  Wait for a message from the COM Port.
*
*******************************************************************************/
int smsg_get_message (unsigned char *bufP, int bufSize, int timeout, bool dump)
{
    int status;
    int i;
    int timeout_count = timeout/10;

    // printf ("smsg_get_message: timeout = %d\n", timeout);

    status = SioEventWait(smsgComP->Port, EV_RXCHAR, timeout);

    // status = SioEvent(smsgComP->Port, (EV_RXCHAR|EV_ERR));

    switch (status) {
    
    case WSC_IO_COMPLETE:

        while (1) {
            status = SioGets(smsgComP->Port, (LPSTR)bufP, bufSize);
            if (status > 0) {
                if (dump) {
                    printf ("\nReceived from com port:\n", status);
                    for (i=0;i<status;i++) {
                        printf ("0x%2.2x ",bufP[i]);
                    }
                    printf ("\n");
                }
                break;
            } else {
                Sleep(100);
                timeout_count--;
                if (timeout_count <= 0) {
                    break;
                }
            }
        }
        break;

    case WSC_IO_PENDING:
        status = -1;
        printf ("COM INFO: WSC_IO_PENDING\n");
        break;

    case  WSC_IO_ERROR:
        status = -1;
        printf ("COM ERROR: WSC_IO_ERROR\n");
        break;

    case  WSC_IE_NOPEN:
        status = -1;
        printf ("COM ERROR: WSC_IE_NOPEN\n");
        break;

    case  WSC_IE_BADID:
        status = -1;
        printf ("COM ERROR: WSC_IE_BADID\n");
        break;

    default:
        status = -1;
        printf ("COM ERROR: Status is %d\n", status);
        break;

    }

    return(status);
}

/*******************************************************************************
*
*
*
*******************************************************************************/
void smsg_rx_flush (void)
{
    SioRxClear (smsgComP->Port);
}


/*******************************************************************************
*
*
*
*******************************************************************************/
static char smsg_error_string[512];
static int smsg_error_check(int Code)
{
    if (Code<0) {
        long WinErr;
        printf("ERROR %d ",Code);
        if (Code==WSC_WIN32ERR) {
            WinErr = SioWinError((LPSTR)smsg_error_string,80);
            printf("(%d): %s\n",WinErr,(LPSTR)smsg_error_string);
        } else {
            printf("\n");
        }

        SioDone(smsgComP->Port);

        exit (0);
    }

    return Code;
}

/*******************************************************************************
*
*
*
*******************************************************************************/
int smsg_get_rxque_level (void)
{
    return SioRxQue(smsgComP->Port);
}

