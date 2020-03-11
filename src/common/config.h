//config.h - main configuration file
#ifndef _CONFIG_H
#define _CONFIG_H

#include "printers.h"

#include <avr/pgmspace.h>

//--------------------------------------
//board revisions
#define OLIMEX_E407      0 //Olimex STM32-E407 devboard (discontinued 1.9.2019)
#define A3IDES2130_REV01 1 //A3ides with TMC2130 rev01 (discontinued 1.9.2019)
#define A3IDES2209_REV01 2 //A3ides with TMC2209 rev01 (discontinued 1.9.2019)
#define A3IDES2209_REV02 4 //A3ides with TMC2209 rev02
#define A3IDES2209_REV03 6 //A3ides with TMC2209 rev03
//simulators
#define A3IDES2209_SIM -2 //SDL simulator 2209 (windows)
#define A3IDES2130_SIM -1 //SDL simulator 2130 (windows)

#ifdef MOTHERBOARD
    #if (MOTHERBOARD == -2)
        #define BOARD A3IDES2209_SIM
    #elif (MOTHERBOARD == -1)
        #define BOARD A3IDES2130_SIM
    #elif (MOTHERBOARD == 0)
        #define BOARD OLIMEX_E407
    #elif (MOTHERBOARD == 1820)
        #define BOARD A3IDES2130_REV01
    #elif (MOTHERBOARD == 1821)
        #define BOARD A3IDES2209_REV01
    #elif (MOTHERBOARD == 1823)
        #define BOARD A3IDES2209_REV02
    #elif (MOTHERBOARD == 1850)
        #define BOARD A3IDES2209_REV02
    #elif (MOTHERBOARD == 1851)
        #define BOARD A3IDES2209_REV02
    #endif
#endif //MOTHERBOARD

//#define BUDDY_XL_ELECTRONICS // Define this for usage of Buddy XL electronics !! YOU CAN DO BETTER MICHAL !!

#ifdef BOARD

    #if (BOARD == A3IDES2209_SIM)
        #include "config_a3ides2209_02.h"
    #elif ((BOARD < A3IDES2209_REV02) || (BOARD > A3IDES2209_REV03))
        #error "BOARD not supported"
    //#elif (BOARD == OLIMEX_E407)
    //#include "config_olimex.h"
    //#elif (BOARD == A3IDES2130_REV01)
    //#include "config_a3ides2130.h"
    //#elif (BOARD == A3IDES2209_REV01)
    //#include "config_a3ides2209.h"
    #elif (BOARD == A3IDES2209_REV02)
        #include "config_a3ides2209_02.h"
    #elif (BOARD == A3IDES2209_REV03)
        #include "config_a3ides2209_02.h"
    #endif

#else
    #error "BOARD not defined"
#endif

//--------------------------------------
//BUDDY_ENABLE_ETHERNET configuration
#ifdef BUDDY_ENABLE_WUI
    #define BUDDY_ENABLE_ETHERNET
#endif //BUDDY_ENABLE_WUI
//--------------------------------------
// Disable wizard startup check on splash screen
#define DISABLE_WIZARD_CHECK_STARTUP

//--------------------------------------
//LCDSIM configuration

//defined as external
#ifdef LCDSIM
    #undef BUDDY_ENABLE_ETHERNET
    #define LCDSIM_COLS 20
    #define LCDSIM_ROWS 4
#endif //LCDSIM

//marlin api config
#define MARLIN_MAX_CLIENTS 3   // maximum number of clients registered in same time
#define MARLIN_MAX_REQUEST 100 // maximum request length in chars

//display PSOD instead of BSOD
//#define PSOD_BSOD

//PID calibration service screen (disabled for XL)
//! @todo what about iXL? Shoudn't it be enabled everywhere and only bed PID disabled using PIDBEDTEMP macro?
#if (PRINTER_TYPE != PRINTER_PRUSA_XL)
    #define PIDCALIBRATION
#endif

//Enabled Z calibration (MK3, MK4, XL)
#if ((PRINTER_TYPE == PRINTER_PRUSA_MK3) || (PRINTER_TYPE == PRINTER_PRUSA_MK4) || (PRINTER_TYPE == PRINTER_PRUSA_XL))
    #define WIZARD_Z_CALIBRATION
#endif

//guiconfig.h included with config
#include "guiconfig.h"

//resource.h included with config
#include "resource.h"

#endif //_CONFIG_H
