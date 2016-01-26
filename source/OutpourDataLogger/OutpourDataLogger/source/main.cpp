/** 
*  @file main.cpp
*  Slamball PC test program for firmware development.
*  This module contains the main entry function for the program.
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

#include "getopt.h"
#include "dataLogger_exec.hpp"

static void process_input_args(int argc, TCHAR **argv, OpLogger *opLoggerP);
static void show_help(void);
static BOOL WINAPI ConsoleHandler(DWORD);

// Instantiate our logger object
OpLogger opLogger;

/*******************************************************************************
*
* Main entry function
*
*******************************************************************************/
int main(int argc, char **argv) {

    printf("\n\nOutpour DataLogger\n");
    printf("Version 0.03, Built on %s at %s\n\n", __DATE__, __TIME__);

    if (argc > 1) {
        process_input_args(argc, argv, &opLogger);
    } else {
        int comport;
        comport = smsg_find_first_com_port();
        if (comport >= 0) {
            opLogger.setComport(comport + 1);
        } else {
            printf("\nCOM PORT %d NOT FOUND\n", comport);
            printf("hit any key to exit.\n", comport);
            while (!_kbhit()) {
                Sleep(100);
            }

            return -1;
        }
    }

    // Setup the handler for getting the control-c from console.
    if (!SetConsoleCtrlHandler((PHANDLER_ROUTINE)ConsoleHandler, TRUE)) {
        fprintf(stderr, "Unable to install handler!\n");
        return EXIT_FAILURE;
    }

    opLogger.Exec();
}

/******************************************************************************
* 
*  Command Line Option Processing                                                                            
*                                                                              
******************************************************************************/
// Option ID's used with longOpt parameter declaration
enum {
    OPTION_HELP,
    OPTION_FIND_COM_PORTS,
    OPTION_COM_PORT,
};

/**
 *
 * Process the command line arguments 
 * 
 * @param int argc     Arg count from main entry
 * @param char** argv  Arg array from main entry
 */
static void process_input_args(int argc, TCHAR **argv, OpLogger *opLoggerP) {
    int opt;
    int longIndex;
    std::string regNameString;

    static const struct option longOpts[] = {
        { "help", no_argument, NULL, OPTION_HELP},{ "h", no_argument, NULL, OPTION_HELP},{ "listcom", no_argument, NULL, OPTION_FIND_COM_PORTS},{ "port", required_argument, NULL, OPTION_COM_PORT},
    };

    // Process input arguments
    opt = getopt_long(argc, argv, "", longOpts, &longIndex);
    while (opt != -1) {
        switch (opt) {
        case OPTION_FIND_COM_PORTS:
            smsg_find_com_ports();
            exit(0);
            break;
        case OPTION_COM_PORT:
            opLoggerP->setComport(atoi(optarg));
            break;
        case OPTION_HELP:
            show_help();
            exit(0);
            break;
        default:
            exit(0);
            break;
        }

        opt = getopt_long(argc, argv, "", longOpts, &longIndex);
    }
}

/**
 *
 * Print options to console output.
 * 
 */
static void show_help(void) {
    std::string msg;

    msg.clear();
    msg.append("PC Tester most common usage: \"opLogger --port=port_number\"");
    std::cout << std::endl << msg << std::endl << std::endl;

    msg.clear();
    msg.append("option: --listcom");
    std::cout << msg << std::endl;
    msg.clear();
    msg.append("List existing com ports in the system.\n");
    msg.append("Example usage:  opLogger.exe --listcom ");
    std::cout << msg << std::endl << std::endl;
}

/**
 * 
 * \brief Handle Control-C from the console
 * 
 * @param dwType 
 * 
 * @return BOOL 
 */
static BOOL WINAPI ConsoleHandler(DWORD dwType) {
    switch (dwType) {
    case CTRL_C_EVENT:
        printf("Got ctrl-c....\n");
        opLogger.Abort();
        break;
    case CTRL_BREAK_EVENT:
        printf("break\n");
        break;
    default:
        printf("Some other event\n");
    }
    return TRUE;
}
