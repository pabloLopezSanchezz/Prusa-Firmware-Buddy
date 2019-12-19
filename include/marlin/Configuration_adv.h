//Configuration_adv.h
#ifndef _CONFIGURATION_ADV_H
#define _CONFIGURATION_ADV_H

#include "config.h"

#if (MOTHERBOARD == 1820)
    //discontinued
    #error "Board not supported!"

    #if (PRINTER_TYPE == PRINTER_PRUSA_MK3)
        #include "Configuration_A3ides_2130_MK3_adv.h"
    #elif (PRINTER_TYPE == PRINTER_PRUSA_MINI)
        #include "Configuration_A3ides_2130_MINI_adv.h"
    #else
        #error "Unknown PRINTER_TYPE!"
    #endif

#elif (MOTHERBOARD == 1821)

    #if (PRINTER_TYPE == PRINTER_PRUSA_MK3)
        #include "Configuration_A3ides_2209_MK3_adv.h"
    #elif (PRINTER_TYPE == PRINTER_PRUSA_MINI)
        #include "Configuration_A3ides_2209_MINI_adv.h"
    #elif (PRINTER_TYPE == PRINTER_PRUSA_XL)
        #include "Configuration_A3ides_2209_XL_adv.h"
    #elif (PRINTER_TYPE == PRINTER_PRUSA_MK4)
        #include "Configuration_A3ides_2209_MK4_adv.h"
    #else
        #error "Unknown PRINTER_TYPE!"
    #endif

#elif (MOTHERBOARD == 1823)

    #if (PRINTER_TYPE == PRINTER_PRUSA_MK3)
        #include "Configuration_A3ides_2209_MK3_adv.h"
    #elif (PRINTER_TYPE == PRINTER_PRUSA_MINI)
        #include "Configuration_A3ides_2209_MINI_adv.h"
    #elif (PRINTER_TYPE == PRINTER_PRUSA_XL)
        #include "Configuration_A3ides_2209_XL_adv.h"
    #elif (PRINTER_TYPE == PRINTER_PRUSA_MK4)
        #include "Configuration_A3ides_2209_MK4_adv.h"
    #else
        #error "Unknown PRINTER_TYPE!"
    #endif

#else
    #error "Unknown MOTHERBOARD!"
#endif

#endif //_CONFIGURATION_ADV_H
