//config.h - main configuration file
#pragma once

#include "printers.h"
#include <avr/pgmspace.h>
#include "config_a3ides2209_02.h"

//--------------------------------------
//BUDDY_ENABLE_ETHERNET configuration
#ifdef BUDDY_ENABLE_WUI
    #define BUDDY_ENABLE_ETHERNET
#endif //BUDDY_ENABLE_WUI

// Disable wizard startup check on splash screen
#define DISABLE_WIZARD_CHECK_STARTUP

//marlin api config
#define MARLIN_MAX_CLIENTS  3   // maximum number of clients registered in same time
#define MARLIN_MAX_REQUEST  100 // maximum request length in chars
#define MARLIN_SERVER_QUEUE 128 // size of marlin server input character queue (number of characters)
#define MARLIN_CLIENT_QUEUE 16  // size of marlin client input message queue (number of messages)

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

//CRC32 config - use hardware CRC32 with RTOS
#define CRC32_USE_HW
#define CRC32_USE_RTOS

//guiconfig.h included with config
#include "guiconfig.h"

//resource.h included with config
#include "resource.h"
