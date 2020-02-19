#pragma once
#include "printers.h"

#if (!defined(PRINTER_PRUSA_MINI) || !defined(PRINTER_PRUSA_MK4)             \
    || !defined(PRINTER_PRUSA_XL) || !defined(PRINTER_PRUSA_IXL)             \
    || !defined(PRINTER_PRUSA_MANIPULATOR) || !defined(PRINTER_PRUSA_PICKER) \
    || !defined(PRINTER_PRUSA_EXTRACTOR))
    #error "Some printer type not defined."
#endif
#if (PRINTER_TYPE == PRINTER_PRUSA_MINI)
    #include "main_MINI.h"
#elif (PRINTER_TYPE == PRINTER_PRUSA_MK4)
    #include "main_MK4.h"
#elif (PRINTER_TYPE == PRINTER_PRUSA_XL)
    #include "main_XL.h"
#elif (PRINTER_TYPE == PRINTER_PRUSA_IXL)
    #include "main_IXL.h"
#elif (PRINTER_TYPE == PRINTER_PRUSA_MANIPULATOR)
    #include "main_MANIPULATOR.h"
#elif (PRINTER_TYPE == PRINTER_PRUSA_PICKER)
    #include "main_PICKER.h"
#elif (PRINTER_TYPE == PRINTER_PRUSA_EXTRACTOR)
    #include "main_EXTRACTOR.h"
#else
    #error "Unknown PRINTER_TYPE!"
#endif
