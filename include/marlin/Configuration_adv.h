//! @file
#pragma once

#include "config.h"

#if ((MOTHERBOARD == 1821) || (MOTHERBOARD == 1823) || (MOTHERBOARD == 1850) || (MOTHERBOARD == 1851))
    #if (PRINTER_TYPE == PRINTER_PRUSA_MINI)
        #include "Configuration_A3ides_2209_MINI_adv.h"
    #elif (PRINTER_TYPE == PRINTER_PRUSA_MK3)
        #include "Configuration_A3ides_2209_MK3_adv.h"
    #elif (PRINTER_TYPE == PRINTER_PRUSA_XL)
        #include "Configuration_A3ides_2209_XL_adv.h"
    #elif (PRINTER_TYPE == PRINTER_PRUSA_MK4)
        #include "Configuration_A3ides_2209_MK4_adv.h"
    #elif (PRINTER_TYPE == PRINTER_PRUSA_IXL)
        #include "Configuration_A3ides_2209_iXL_adv.h"
    #elif (PRINTER_TYPE == PRINTER_PRUSA_MANIPULATOR)
        #include "Configuration_A3ides_2209_Manipulator_adv.h"
    #elif (PRINTER_TYPE == PRINTER_PRUSA_PICKER)
        #include "Configuration_A3ides_2209_Picker_adv.h"
    #elif (PRINTER_TYPE == PRINTER_PRUSA_EXTRACTOR)
        #include "Configuration_A3ides_2209_Extractor_adv.h"
    #else
        #error "Unknown PRINTER_TYPE!"
    #endif
#else
    #error "Unknown MOTHERBOARD!"
#endif
