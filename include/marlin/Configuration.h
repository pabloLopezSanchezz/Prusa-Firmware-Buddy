//! @file
#pragma once

#include "config.h"

#if (PRINTER_TYPE == PRINTER_PRUSA_MINI)
    #include "Configuration_A3ides_2209_MINI.h"
#elif (PRINTER_TYPE == PRINTER_PRUSA_MK3)
    #include "Configuration_A3ides_2209_MK3.h"
#elif (PRINTER_TYPE == PRINTER_PRUSA_XL)
    #include "Configuration_A3ides_2209_XL.h"
#elif (PRINTER_TYPE == PRINTER_PRUSA_MK4)
    #include "Configuration_A3ides_2209_MK4.h"
#elif (PRINTER_TYPE == PRINTER_PRUSA_IXL)
    #include "Configuration_A3ides_2209_iXL.h"
#elif (PRINTER_TYPE == PRINTER_PRUSA_MANIPULATOR)
    #include "Configuration_A3ides_2209_Manipulator.h"
#elif (PRINTER_TYPE == PRINTER_PRUSA_PICKER)
    #include "Configuration_A3ides_2209_Picker.h"
#elif (PRINTER_TYPE == PRINTER_PRUSA_EXTRACTOR)
    #include "Configuration_A3ides_2209_Extractor.h"
#else
    #error "Unknown PRINTER_TYPE!"
#endif

#ifdef _EXTUI
    #undef RA_CONTROL_PANEL
    #define EXTENSIBLE_UI
    #define NO_LCD_MENUS
#endif
