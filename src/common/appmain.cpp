//appmain.cpp - arduino-like app start

#include "app.h"
#include "app_metrics.h"
#include "dbg.h"
#include "cmsis_os.h"
#include "config.h"
#include "dbg.h"
#include "adc.h"
#include "jogwheel.h"
#include "hwio.h"
#include "sys.h"
#include "gpio.h"
#include "sound_C_wrapper.h"
#include "metric.h"
#include "cpu_utils.h"

#ifdef LOADCELL_HX711
    #include "loadcell.h"
    #include "hx711.h"
    #include "filament_sensor.h"
#endif //LOADCELL_HX711

#ifdef ADC_EXT_MUX
    #include "advanced_power.h"
#endif

#ifdef SIM_HEATER
    #include "sim_heater.h"
#endif //SIM_HEATER

#ifdef SIM_MOTION
    #include "sim_motion.h"
#endif //SIM_MOTION

#include "uartslave.h"
#include "marlin_server.h"
#include "bsod.h"
#include "eeprom.h"
#include "diag.h"
#include "safe_state.h"
#include "crc32.h"

#include <Arduino.h>
#include "trinamic.h"
#include "../Marlin/src/module/configuration_store.h"
#include "../Marlin/src/module/temperature.h"
#include "../Marlin/src/module/probe.h"

#define DBG _dbg0 //debug level 0
//#define DBG(...)  //disable debug

extern void USBSerial_put_rx_data(uint8_t *buffer, uint32_t length);

extern void reset_trinamic_drivers();

extern "C" {

metric_t metric_app_start = METRIC("app_start", METRIC_VALUE_EVENT, 0, METRIC_HANDLER_ENABLE_ALL);
metric_t metric_maintask_event = METRIC("maintask_loop", METRIC_VALUE_EVENT, 0, METRIC_HANDLER_DISABLE_ALL);
metric_t metric_cpu_usage = METRIC("cpu_usage", METRIC_VALUE_INTEGER, 1000, METRIC_HANDLER_ENABLE_ALL);

extern uartrxbuff_t uart6rxbuff; // PUT rx buffer
extern uartslave_t uart6slave;   // PUT slave

#ifdef BUDDY_ENABLE_ETHERNET
extern osThreadId webServerTaskHandle; // Webserver thread(used for fast boot mode)
#endif                                 //BUDDY_ENABLE_ETHERNET

#ifndef _DEBUG
extern IWDG_HandleTypeDef hiwdg; //watchdog handle
#endif                           //_DEBUG

void app_setup(void) {
    metric_record_event(&metric_app_start);

    setup();

    marlin_server_settings_load(); // load marlin variables from eeprom

    if (INIT_TRINAMIC_FROM_MARLIN_ONLY == 0) {
        init_tmc();
    }
    //DBG("after init_tmc (%ld ms)", HAL_GetTick());

#ifdef LOADCELL_HX711
    hx711.Configure(LOADCELL_PIN_DOUT, LOADCELL_PIN_SCK);
    loadcell.SetScale(eeprom_get_var(EEVAR_LOADCELL_SCALE).flt);
    loadcell.SetThreshold(eeprom_get_var(EEVAR_LOADCELL_THRS).flt);
    loadcell.SetHysteresis(eeprom_get_var(EEVAR_LOADCELL_HYST).flt);
    loadcell.ConfigureSignalEvent(osThreadGetId(), 0x0A);
#endif //LOADCELL_HX711
}

void app_idle(void) {
    Buddy::Metrics::RecordMarlinVariables();
    Buddy::Metrics::RecordRuntimeStats();
    osDelay(0); // switch to other threads - without this is UI slow during printing
}

void app_run(void) {
    DBG("app_run");

#ifdef BUDDY_ENABLE_ETHERNET
    if (diag_fastboot)
        osThreadResume(webServerTaskHandle);
#endif //BUDDY_ENABLE_ETHERNET

    crc32_init();

    uint8_t defaults_loaded = eeprom_init();

    marlin_server_init();
    marlin_server_idle_cb = app_idle;

    adc_init();

#ifdef ADC_EXT_MUX
    adc_reset_fault_signal();
#endif

#ifdef SIM_HEATER
    sim_heater_init();
#endif //SIM_HEATER

    //DBG("before setup (%ld ms)", HAL_GetTick());
    if (diag_fastboot || (!sys_fw_is_valid())) {
        marlin_server_stop_processing();
        if (!sys_fw_is_valid()) // following code will be done only with invalidated firmware
        {
            hwio_safe_state(); // safe states
            for (int i = 0; i < hwio_fan_get_cnt(); ++i)
                hwio_fan_set_pwm(i, 0); // disable fans
        }
        reset_trinamic_drivers();
        if (INIT_TRINAMIC_FROM_MARLIN_ONLY == 0) {
            init_tmc();
        }
    } else
        app_setup();
    //DBG("after setup (%ld ms)", HAL_GetTick());

    if (defaults_loaded && marlin_server_processing()) {
        settings.reset();
#ifndef _DEBUG
        HAL_IWDG_Refresh(&hiwdg);
#endif //_DEBUG
        settings.save();
#ifndef _DEBUG
        HAL_IWDG_Refresh(&hiwdg);
#endif //_DEBUG
    }

    while (1) {
        metric_record_event(&metric_maintask_event);
        metric_record_integer(&metric_cpu_usage, osGetCPUUsage());
        if (marlin_server_processing()) {
            loop();
        }
        uartslave_cycle(&uart6slave);
        marlin_server_loop();
        app_usbhost_reenum();
        osDelay(0); // switch to other threads - without this is UI slow
#ifdef JOGWHEEL_TRACE
        static int signals = jogwheel_signals;
        if (signals != jogwheel_signals) {
            signals = jogwheel_signals;
            DBG("%d %d", signals, jogwheel_encoder);
        }
#endif //JOGWHEEL_TRACE
#ifdef LOADCELL_TRACE
        static int count = loadcell_count;
        if (count != loadcell_count) {
            count = loadcell_count;
            DBG("%d %.1f %d", loadcell_probe, loadcell_load, loadcell_error);
        }
#endif //LOADCELL_TRACE
#ifdef SIM_MOTION_TRACE_X
        static int32_t x = sim_motion_pos[0];
        if (x != sim_motion_pos[0]) {
            x = sim_motion_pos[0];
            DBG("X:%li", x);
        }
#endif //SIM_MOTION_TRACE_X
#ifdef SIM_MOTION_TRACE_Y
        static int32_t y = sim_motion_pos[1];
        if (y != sim_motion_pos[1]) {
            y = sim_motion_pos[1];
            DBG("Y:%li", y);
        }
#endif //SIM_MOTION_TRACE_Y
#ifdef SIM_MOTION_TRACE_Z
        static int32_t z = sim_motion_pos[2];
        if (z != sim_motion_pos[2]) {
            z = sim_motion_pos[2];
            DBG("Z:%li", z);
        }
#endif //SIM_MOTION_TRACE_Z
    }
}

void app_error(void) {
    bsod("app_error");
}

void app_assert(uint8_t *file, uint32_t line) {
    bsod("app_assert");
}

void app_cdc_rx(uint8_t *buffer, uint32_t length) {
    USBSerial_put_rx_data(buffer, length);
}

#ifdef LOADCELL_HX711
void hx711_irq() {
    if (!hx711.IsValueReady())
        return;

    static int sample_counter = 0;
    int32_t raw_value;

    static HX711::Channel current_channel = hx711.CHANNEL_A_GAIN_128;
    HX711::Channel next_channel;

    sample_counter += 1;

    if (!(loadcell.IsHighPrecisionEnabled()) && sample_counter % 13 == 0) {
        next_channel = hx711.CHANNEL_B_GAIN_32;
    } else {
        next_channel = hx711.CHANNEL_A_GAIN_128;
    }

    raw_value = hx711.ReadValue(next_channel);

    if (current_channel == hx711.CHANNEL_A_GAIN_128) {
        loadcell.ProcessSample(raw_value);
    } else {
        fs_process_sample(raw_value);
    }
    current_channel = next_channel;
}
#endif //LOADCELL_HX711

#ifdef ADC_EXT_MUX
void advanced_power_irq() {
    static uint8_t cnt_advanced_power_update = 0;
    if (++cnt_advanced_power_update >= 70) { // update Advanced power variables = 14Hz
        advancedpower.Update();
        Buddy::Metrics::RecordPowerStats();
        cnt_advanced_power_update = 0;
    }
}
#endif //ADC_EXT_MUX

void adc_tick_1ms(void) {
    adc_cycle();
#ifdef ADC_EXT_MUX
    adc2_cycle();
#endif
#ifdef SIM_HEATER
    static uint8_t cnt_sim_heater = 0;
    if (++cnt_sim_heater >= 50) // sim_heater freq = 20Hz
    {
        sim_heater_cycle();
        cnt_sim_heater = 0;
    }
#endif //SIM_HEATER
#ifdef LOADCELL_HX711
    hx711_irq();
#endif //LOADCELL_HX711

#ifdef SIM_MOTION
    sim_motion_cycle();
#endif //SIM_MOTION

#ifdef ADC_EXT_MUX
    advanced_power_irq();
#endif
}

void app_tim14_tick(void) {
#ifndef HAS_GUI
    #error "HAS_GUI not defined."
#elif HAS_GUI
    jogwheel_update_1ms();
#endif
    Sound_Update1ms();
    //hwio_update_1ms();
    adc_tick_1ms();
}

#include "usbh_core.h"
extern USBH_HandleTypeDef hUsbHostHS; // UsbHost handle

// Re-enumerate UsbHost in case that it hangs in enumeration state (HOST_ENUMERATION,ENUM_IDLE)
// this is not solved in original UsbHost driver
// this occurs e.g. when user connects and then quickly disconnects usb flash during connection process
// state is checked every 100ms, timeout for re-enumeration is 500ms
// TODO: maybe we will change condition for states, because it can hang also in different state
void app_usbhost_reenum(void) {
    static uint32_t timer = 0;     // static timer variable
    uint32_t tick = HAL_GetTick(); // read tick
    if ((tick - timer) > 100) {    // every 100ms
        // timer is valid, UsbHost is in enumeration state
        if ((timer) && (hUsbHostHS.gState == HOST_ENUMERATION) && (hUsbHostHS.EnumState == ENUM_IDLE)) {
            // longer than 500ms
            if ((tick - timer) > 500) {
                _dbg("USB host reenumerating"); // trace
                USBH_ReEnumerate(&hUsbHostHS);  // re-enumerate UsbHost
            }
        } else // otherwise update timer
            timer = tick;
    }
}

} // extern "C"

//cpp code
