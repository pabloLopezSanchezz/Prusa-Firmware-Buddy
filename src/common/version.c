#include "version.h"
#include "config.h"

#define _STR(x) #x
#define STR(x) _STR(x)

const char project_version[] = STR(FW_VERSION);

const char project_version_full[] = STR(FW_VERSION_FULL);

const char project_version_suffix[] = STR(FW_VERSION_SUFFIX);

const char project_version_suffix_short[] = STR(FW_VERSION_SUFFIX_SHORT);

const int project_build_number = FW_BUILD_NUMBER;

#if (PRINTER_TYPE == PRINTER_PRUSA_MINI)
    const char project_firmware_name[] = "Buddy_MINI";
#elif (PRINTER_TYPE == PRINTER_PRUSA_XL)
    const char project_firmware_name[] = "Buddy_XL";
#elif (PRINTER_TYPE == PRINTER_PRUSA_MK4)
    const char project_firmware_name[] = "Buddy_MK4";
#elif (PRINTER_TYPE == PRINTER_PRUSA_IXL)
    const char project_firmware_name[] = "Buddy_iXL";
#elif (PRINTER_TYPE == PRINTER_PRUSA_MANIPULATOR)
    const char project_firmware_name[] = "Buddy_Manipulator";
#elif (PRINTER_TYPE == PRINTER_PRUSA_PICKER)
    const char project_firmware_name[] = "Buddy_Picker";
#elif (PRINTER_TYPE == PRINTER_PRUSA_EXTRACTOR)
    const char project_firmware_name[] = "Buddy_Extractor";
#else
    #error "Unknown PRINTER_TYPE."
#endif
