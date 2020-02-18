#pragma once
#include "printers.h"

#if (!defined(PRINTER_PRUSA_MINI) || !defined(PRINTER_PRUSA_MK4)             \
    || !defined(PRINTER_PRUSA_XL) || !defined(PRINTER_PRUSA_IXL)             \
    || !defined(PRINTER_PRUSA_MANIPULATOR) || !defined(PRINTER_PRUSA_PICKER) \
    || !defined(PRINTER_PRUSA_EXTRACTOR))
    #error "Some printer type not defined."
#endif
#if (PRINTER_TYPE == PRINTER_PRUSA_MINI)
    #include "hwio_pindef_MINI.h"
#elif (PRINTER_TYPE == PRINTER_PRUSA_MK4)
    #include "hwio_pindef_MK4.h"
#elif (PRINTER_TYPE == PRINTER_PRUSA_XL)
    #include "hwio_pindef_XL.h"
#elif (PRINTER_TYPE == PRINTER_PRUSA_IXL)
    #include "hwio_pindef_IXL.h"
#elif (PRINTER_TYPE == PRINTER_PRUSA_MANIPULATOR)
    #include "hwio_pindef_MANIPULATOR.h"
#elif (PRINTER_TYPE == PRINTER_PRUSA_PICKER)
    #include "hwio_pindef_PICKER.h"
#elif (PRINTER_TYPE == PRINTER_PRUSA_EXTRACTOR)
    #include "hwio_pindef_EXTRACTOR.h"
#else
    #error "Unknown PRINTER_TYPE!"
#endif
