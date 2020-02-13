#include "../../lib/Marlin/Marlin/src/inc/MarlinConfig.h"

#include "../../lib/Marlin/Marlin/src/gcode/gcode.h"
#include "M330.h"

bool GcodeSuite::process_parsed_command_custom(bool no_ok) {
    switch (parser.command_letter) {
    case 'M':
        switch (parser.codenum) {
        case 330:
            PrusaGcodeSuite::M330();
            break;
        case 331:
            PrusaGcodeSuite::M331();
            break;
        case 332:
            PrusaGcodeSuite::M332();
            break;
        case 333:
            PrusaGcodeSuite::M333();
            break;
        }
        break;
    default:
        return false;
    }
}
