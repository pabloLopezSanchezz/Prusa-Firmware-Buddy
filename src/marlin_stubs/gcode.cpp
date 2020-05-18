#include "../../lib/Marlin/Marlin/src/inc/MarlinConfig.h"

#include "../../lib/Marlin/Marlin/src/gcode/gcode.h"
#include "M330.h"
#include "metric.h"

#ifdef LOADCELL_HX711
    #include "loadcell.h"
#endif

static void record_pre_gcode_metrics();

bool GcodeSuite::process_parsed_command_custom(bool no_ok) {
    record_pre_gcode_metrics();

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
        }
        return false;
    default:
        return false;
    }
}

static void record_pre_gcode_metrics() {
    static metric_t gcode = METRIC("gcode", METRIC_VALUE_STRING, 0, METRIC_HANDLER_DISABLE_ALL);
    metric_record_string(&gcode, "%s", parser.command_ptr);

#ifdef LOADCELL_HX711
    static metric_t loadcell_scale_m = METRIC("loadcell_scale", METRIC_VALUE_FLOAT, 1000, METRIC_HANDLER_ENABLE_ALL);
    static metric_t loadcell_threshold_static_m = METRIC("loadcell_threshold", METRIC_VALUE_FLOAT, 1000, METRIC_HANDLER_ENABLE_ALL);
    static metric_t loadcell_threshold_continuous_m = METRIC("loadcell_threshold_cont", METRIC_VALUE_FLOAT, 1000, METRIC_HANDLER_ENABLE_ALL);
    static metric_t loadcell_hysteresis_m = METRIC("loadcell_hysteresis", METRIC_VALUE_FLOAT, 1000, METRIC_HANDLER_ENABLE_ALL);
    metric_register(&loadcell_scale_m);
    metric_register(&loadcell_threshold_static_m);
    metric_register(&loadcell_threshold_continuous_m);
    metric_register(&loadcell_hysteresis_m);

    if (parser.command_letter == 'G' && parser.codenum == 29) {
        // log loadcell settings on beginning of G29
        metric_record_float(&loadcell_scale_m, loadcell.GetScale());
        metric_record_float(&loadcell_threshold_static_m, loadcell.GetThreshold(Loadcell::TareMode::Static));
        metric_record_float(&loadcell_threshold_continuous_m, loadcell.GetThreshold(Loadcell::TareMode::Continuous));
        metric_record_float(&loadcell_hysteresis_m, loadcell.GetHysteresis());
    }
#endif
}
