#include "app_metrics.h"
#include "metric.h"

#include "../Marlin/src/module/temperature.h"
#include "../Marlin/src/module/planner.h"
#include "../Marlin/src/module/stepper.h"

/// This metric is defined in Marlin/src/module/probe.cpp, thus no interface
extern metric_t metric_probe_z;

void Buddy::Metrics::RecordMarlinVariables() {
    metric_register(&metric_probe_z);

#if HAS_TEMP_HEATBREAK
    static metric_t heatbreak = METRIC("temp_hbr", METRIC_VALUE_FLOAT, 1000 - 8, METRIC_HANDLER_DISABLE_ALL);
    metric_record_float(&heatbreak, thermalManager.degHeatbreak());
#endif

    static metric_t bed = METRIC("temp_bed", METRIC_VALUE_FLOAT, 2000 + 23, METRIC_HANDLER_DISABLE_ALL);
    metric_record_float(&bed, thermalManager.degBed());

    static metric_t target_bed = METRIC("ttemp_bed", METRIC_VALUE_INTEGER, 1000, METRIC_HANDLER_DISABLE_ALL);
    metric_record_integer(&target_bed, thermalManager.degTargetBed());

    static metric_t nozzle = METRIC("temp_noz", METRIC_VALUE_FLOAT, 1000 - 10, METRIC_HANDLER_DISABLE_ALL);
    metric_record_float(&nozzle, thermalManager.degHotend(0));

    static metric_t target_nozzle = METRIC("ttemp_noz", METRIC_VALUE_INTEGER, 1000 + 7, METRIC_HANDLER_DISABLE_ALL);
    metric_record_integer(&target_nozzle, thermalManager.degTargetHotend(0));

    static metric_t fan_speed = METRIC("fan_speed", METRIC_VALUE_INTEGER, 501, METRIC_HANDLER_DISABLE_ALL);
    metric_record_integer(&fan_speed, thermalManager.fan_speed[0]);

    static metric_t ipos_x = METRIC("ipos_x", METRIC_VALUE_INTEGER, 10, METRIC_HANDLER_DISABLE_ALL);
    metric_record_integer(&ipos_x, stepper.position(AxisEnum::X_AXIS));
    static metric_t ipos_y = METRIC("ipos_y", METRIC_VALUE_INTEGER, 10, METRIC_HANDLER_DISABLE_ALL);
    metric_record_integer(&ipos_y, stepper.position(AxisEnum::Y_AXIS));
    static metric_t ipos_z = METRIC("ipos_z", METRIC_VALUE_INTEGER, 10, METRIC_HANDLER_DISABLE_ALL);
    metric_record_integer(&ipos_z, stepper.position(AxisEnum::Z_AXIS));

    static metric_t pos_x = METRIC("pos_x", METRIC_VALUE_FLOAT, 11, METRIC_HANDLER_DISABLE_ALL);
    metric_record_float(&pos_x, planner.get_axis_position_mm(AxisEnum::X_AXIS));
    static metric_t pos_y = METRIC("pos_y", METRIC_VALUE_FLOAT, 11, METRIC_HANDLER_DISABLE_ALL);
    metric_record_float(&pos_y, planner.get_axis_position_mm(AxisEnum::Y_AXIS));
    static metric_t pos_z = METRIC("pos_z", METRIC_VALUE_FLOAT, 11, METRIC_HANDLER_DISABLE_ALL);
    metric_record_float(&pos_z, planner.get_axis_position_mm(AxisEnum::Z_AXIS));

}
