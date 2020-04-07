// filament_sensor.c

//there is 10kOhm PU to 5V in filament sensor
//mcu PU/PD is in range 30 - 50 kOhm

//when PD is selected and sensor is connected Vmcu = min 3.75V .. (5V * 30kOhm) / (30 + 10 kOhm)
//pin is 5V tolerant

//mcu has 5pF, transistor D-S max 15pF
//max R is 50kOhm
//Max Tau ~= 20*10^-12 * 50*10^3 = 1*10^-6 s ... about 1us

#include "filament_sensor.h"
#include "hwio_pindef.h" //PIN_FSENSOR
#include "stm32f4xx_hal.h"
#include "gpio.h"
#include "eeprom.h"
#include "FreeRTOS.h"      //must apper before include task.h
#include "task.h"          //critical sections
#include "cmsis_os.h"      //osDelay
#include "marlin_client.h" //enable/disable fs in marlin
#include "metric.h"
#include "config.h"

static metric_t metric_fsensor_raw = METRIC("fsensor_raw", METRIC_VALUE_INTEGER, 0, METRIC_HANDLER_DISABLE_ALL);

static volatile fsensor_t state = FS_NOT_INICIALIZED;
static volatile fsensor_t last_state = FS_NOT_INICIALIZED;

typedef enum {
    M600_on_edge = 0,
    M600_on_level = 1,
    M600_never = 2
} send_M600_on_t;

typedef struct {
    uint8_t M600_sent : 1;
    uint8_t send_M600_on : 2;
} status_t;

static status_t status = { 0, M600_on_edge };

static fsensor_t fsensor_state;

/*---------------------------------------------------------------------------*/
//local functions

//simple filter
//without filter fs_meas_cycle1 could set FS_NO_SENSOR (in case filament just runout)
static void _set_state(fsensor_t st) {
    taskENTER_CRITICAL();
    if (last_state == st)
        state = st;
    last_state = st;
    taskEXIT_CRITICAL();
}

static void _enable() {
    state = FS_NOT_INICIALIZED;
    last_state = FS_NOT_INICIALIZED;
}

static void _disable() {
    state = FS_DISABLED;
    last_state = FS_DISABLED;
}

/*---------------------------------------------------------------------------*/
//global thread safe functions
fsensor_t fs_get_state() {
    return state;
}

//value can change during read, but it is not a problem
int fs_did_filament_runout() {
    return state == FS_NO_FILAMENT;
}

void fs_send_M600_on_edge() {
    status.send_M600_on = M600_on_edge;
}

void fs_send_M600_on_level() {
    status.send_M600_on = M600_on_level;
}

void fs_send_M600_never() {
    status.send_M600_on = M600_never;
}
/*---------------------------------------------------------------------------*/
//global thread safe functions
//but cannot be called from interrupt
void fs_enable() {
    taskENTER_CRITICAL();
    _enable();
    eeprom_set_var(EEVAR_FSENSOR_ENABLED, variant8_ui8(1));
    taskEXIT_CRITICAL();
}

void fs_disable() {
    taskENTER_CRITICAL();
    _disable();
    eeprom_set_var(EEVAR_FSENSOR_ENABLED, variant8_ui8(0));
    taskEXIT_CRITICAL();
}

uint8_t fs_get__send_M600_on__and_disable() {
    taskENTER_CRITICAL();
    uint8_t ret = status.send_M600_on;
    status.send_M600_on = M600_never;
    taskEXIT_CRITICAL();
    return ret;
}
void fs_restore__send_M600_on(uint8_t send_M600_on) {
    taskENTER_CRITICAL();
    //cannot call _init(); - it could cause stacking in unincialized state
    status.send_M600_on = send_M600_on;
    taskEXIT_CRITICAL();
}

fsensor_t fs_wait_inicialized() {
    fsensor_t ret = fs_get_state();
    while (ret == FS_NOT_INICIALIZED) {
        osDelay(0); // switch to other threads
        ret = fs_get_state();
    }
    return ret;
}

void fs_clr_sent() {
    taskENTER_CRITICAL();
    status.M600_sent = 0;
    taskEXIT_CRITICAL();
}

/*---------------------------------------------------------------------------*/
//global not thread safe functions
static void _init() {
    int enabled = eeprom_get_var(EEVAR_FSENSOR_ENABLED).ui8 ? 1 : 0;

    if (enabled)
        _enable();
    else
        _disable();
}

void fs_init_on_edge() {
    _init();
    fs_send_M600_on_edge();
}
void fs_init_on_level() {
    _init();
    fs_send_M600_on_level();
}
void fs_init_never() {
    _init();
    fs_send_M600_never();
}

/*---------------------------------------------------------------------------*/
//methods called only in fs_cycle
static void _injectM600() {
    marlin_vars_t *vars = marlin_update_vars(MARLIN_VAR_MSK(MARLIN_VAR_SD_PRINT) | MARLIN_VAR_MSK(MARLIN_VAR_WAITHEAT) | MARLIN_VAR_MSK(MARLIN_VAR_WAITUSER));
    if (vars->sd_printing && (!vars->wait_user) /*&& (!vars->wait_heat)*/) {
        marlin_gcode_push_front("M600"); //change filament
        status.M600_sent = 1;
    }
}

//dealay between calls must be 1us or longer
void fs_cycle() {
    //sensor is disabled (only init can enable it)
    if (state == FS_DISABLED)
        return;

    int had_filament = state == FS_HAS_FILAMENT ? 1 : 0;

    _set_state(fsensor_state);

    if (status.M600_sent == 0 && state == FS_NO_FILAMENT) {
        switch (status.send_M600_on) {
        case M600_on_edge:
            if (!had_filament)
                break;
            //if had_filament == 1 - do not break
        case M600_on_level:
            _injectM600();
            break;
        case M600_never:
        default:
            break;
        }
    }

    //clear M600_sent status if marlin is paused
    if (marlin_update_vars(MARLIN_VAR_MSK(MARLIN_VAR_WAITUSER))->wait_user) {
        status.M600_sent = 0;
    }
}

void fs_process_sample(int32_t fs_raw_value) {
    metric_record_integer(&metric_fsensor_raw, fs_raw_value);

    if (fs_raw_value <= FILAMENT_SENSOR_HX711_LOW) {
        fsensor_state = FS_NO_FILAMENT;
    } else if (fs_raw_value < 2000) {
        fsensor_state = FS_NOT_CONNECTED;
    } else {
        fsensor_state = FS_HAS_FILAMENT;
    }
}
