#include "app_metrics.h"
#include "metric.h"
#include "version.h"
#include "FreeRTOS.h"
#include "cmsis_os.h"
#include "malloc.h"
#include "heap.h"
#include "advanced_power.h"

#include "../Marlin/src/module/temperature.h"
#include "../Marlin/src/module/planner.h"
#include "../Marlin/src/module/stepper.h"

/// This metric is defined in Marlin/src/module/probe.cpp, thus no interface
#if HAS_BED_PROBE
extern metric_t metric_probe_z;
extern metric_t metric_probe_z_raw;
extern metric_t metric_probe_z_diff;
extern metric_t metric_home_diff;
#endif

void Buddy::Metrics::RecordRuntimeStats() {
    static metric_t fw_version = METRIC("fw_version", METRIC_VALUE_STRING, 1 * 1000, METRIC_HANDLER_ENABLE_ALL);
    metric_record_string(&fw_version, "%s", project_version_full);

    static metric_t stack = METRIC("stack", METRIC_VALUE_CUSTOM, 0, METRIC_HANDLER_ENABLE_ALL);
    static TaskStatus_t task_statuses[11];
    static uint32_t last_recorded_ticks = 0;
    if (HAL_GetTick() - last_recorded_ticks > 3000) {
        int count = uxTaskGetSystemState(task_statuses, sizeof(task_statuses) / sizeof(task_statuses[1]), NULL);
        for (int idx = 0; idx < count; idx++) {
            size_t s = malloc_usable_size(task_statuses[idx].pxStackBase);
            metric_record_custom(&stack, ",n=%.7s t=%i,m=%hu", task_statuses[idx].pcTaskName, s, task_statuses[idx].usStackHighWaterMark);
        }
        last_recorded_ticks = HAL_GetTick();
    }

    static metric_t heap = METRIC("heap", METRIC_VALUE_CUSTOM, 503, METRIC_HANDLER_ENABLE_ALL);
    metric_record_custom(&heap, " free=%ii,total=%ii", xPortGetFreeHeapSize(), heap_total_size);
}

void Buddy::Metrics::RecordMarlinVariables() {
#if HAS_BED_PROBE
    metric_register(&metric_probe_z);
    metric_register(&metric_probe_z_raw);
    metric_register(&metric_probe_z_diff);
    metric_register(&metric_home_diff);
#endif
    static metric_t is_printing = METRIC("is_printing", METRIC_VALUE_INTEGER, 5000, METRIC_HANDLER_ENABLE_ALL);
    metric_record_integer(&is_printing, printingIsActive() ? 1 : 0);

#if HAS_TEMP_HEATBREAK
    static metric_t heatbreak
        = METRIC("temp_hbr", METRIC_VALUE_FLOAT, 1000 - 8, METRIC_HANDLER_DISABLE_ALL);
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

#if FAN_COUNT > 0
    static metric_t fan_speed = METRIC("fan_speed", METRIC_VALUE_INTEGER, 501, METRIC_HANDLER_DISABLE_ALL);
    metric_record_integer(&fan_speed, thermalManager.fan_speed[0]);
#endif

    static metric_t ipos_x = METRIC("ipos_x", METRIC_VALUE_INTEGER, 10, METRIC_HANDLER_DISABLE_ALL);
    metric_record_integer(&ipos_x, stepper.position_from_startup(AxisEnum::X_AXIS));
    static metric_t ipos_y = METRIC("ipos_y", METRIC_VALUE_INTEGER, 10, METRIC_HANDLER_DISABLE_ALL);
    metric_record_integer(&ipos_y, stepper.position_from_startup(AxisEnum::Y_AXIS));
    static metric_t ipos_z = METRIC("ipos_z", METRIC_VALUE_INTEGER, 10, METRIC_HANDLER_DISABLE_ALL);
    metric_record_integer(&ipos_z, stepper.position_from_startup(AxisEnum::Z_AXIS));

    static metric_t pos_x = METRIC("pos_x", METRIC_VALUE_FLOAT, 11, METRIC_HANDLER_DISABLE_ALL);
    metric_record_float(&pos_x, planner.get_axis_position_mm(AxisEnum::X_AXIS));
    static metric_t pos_y = METRIC("pos_y", METRIC_VALUE_FLOAT, 11, METRIC_HANDLER_DISABLE_ALL);
    metric_record_float(&pos_y, planner.get_axis_position_mm(AxisEnum::Y_AXIS));
    static metric_t pos_z = METRIC("pos_z", METRIC_VALUE_FLOAT, 11, METRIC_HANDLER_DISABLE_ALL);
    metric_record_float(&pos_z, planner.get_axis_position_mm(AxisEnum::Z_AXIS));
}

#ifdef ADC_EXT_MUX
void Buddy::Metrics::RecordPowerStats() {

    static metric_t metric_bed_v_raw = METRIC("volt_bed_raw", METRIC_VALUE_INTEGER, 1000, METRIC_HANDLER_DISABLE_ALL);
    metric_record_integer(&metric_bed_v_raw, advancedpower.GetBedVoltageRaw());
    static metric_t metric_bed_v = METRIC("volt_bed", METRIC_VALUE_FLOAT, 1001, METRIC_HANDLER_ENABLE_ALL);
    metric_record_float(&metric_bed_v, advancedpower.GetBedVoltage());
    static metric_t metric_nozzle_v_raw = METRIC("volt_nozz_raw", METRIC_VALUE_INTEGER, 1002, METRIC_HANDLER_DISABLE_ALL);
    metric_record_integer(&metric_nozzle_v_raw, advancedpower.GetHeaterVoltageRaw());
    static metric_t metric_nozzle_v = METRIC("volt_nozz", METRIC_VALUE_FLOAT, 1003, METRIC_HANDLER_ENABLE_ALL);
    metric_record_float(&metric_nozzle_v, advancedpower.GetHeaterVoltage());
    static metric_t metric_nozzle_i_raw = METRIC("curr_nozz_raw", METRIC_VALUE_INTEGER, 1004, METRIC_HANDLER_DISABLE_ALL);
    metric_record_integer(&metric_nozzle_i_raw, advancedpower.GetHeaterCurrentRaw());
    static metric_t metric_nozzle_i = METRIC("curr_nozz", METRIC_VALUE_FLOAT, 1005, METRIC_HANDLER_ENABLE_ALL);
    metric_record_float(&metric_nozzle_i, advancedpower.GetHeaterCurrent());
    static metric_t metric_input_i_raw = METRIC("curr_inp_raw", METRIC_VALUE_INTEGER, 1006, METRIC_HANDLER_DISABLE_ALL);
    metric_record_integer(&metric_input_i_raw, advancedpower.GetInputCurrentRaw());
    static metric_t metric_input_i = METRIC("curr_inp", METRIC_VALUE_FLOAT, 1007, METRIC_HANDLER_ENABLE_ALL);
    metric_record_float(&metric_input_i, advancedpower.GetInputCurrent());
    static metric_t metric_oc_nozzle_fault = METRIC("oc_nozz", METRIC_VALUE_INTEGER, 1008, METRIC_HANDLER_ENABLE_ALL);
    metric_record_integer(&metric_oc_nozzle_fault, advancedpower.HeaterOvercurentFaultDetected());
    static metric_t metric_oc_input_fault = METRIC("oc_inp", METRIC_VALUE_INTEGER, 1009, METRIC_HANDLER_ENABLE_ALL);
    metric_record_integer(&metric_oc_input_fault, advancedpower.OvercurrentFaultDetected());
}
#endif //ADC_EXT_MUX
