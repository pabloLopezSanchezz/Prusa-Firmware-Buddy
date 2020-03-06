#include "../../lib/Marlin/Marlin/src/inc/MarlinConfig.h"

#include "../../lib/Marlin/Marlin/src/gcode/gcode.h"
#include "M330.h"
#include "metric.h"

bool GcodeSuite::process_parsed_command_custom(bool no_ok) {
    {
        static metric_t gcode = METRIC("gcode", METRIC_VALUE_STRING, 0, METRIC_HANDLER_DISABLE_ALL);
        metric_record_string(&gcode, "%s", parser.command_ptr);
    }

    switch (parser.command_letter) {
    case 'M':
        switch (parser.codenum) {
        case 330:
            PrusaGcodeSuite::M330();
            return true;
        case 331:
            PrusaGcodeSuite::M331();
            return true;
        case 332:
            PrusaGcodeSuite::M332();
            return true;
        case 333:
            PrusaGcodeSuite::M333();
            return true;
        case 334:
            PrusaGcodeSuite::M334();
            return true;
        case 335:
            PrusaGcodeSuite::M335();
            return true;
        }
        return false;
    default:
        return false;
    }
}
