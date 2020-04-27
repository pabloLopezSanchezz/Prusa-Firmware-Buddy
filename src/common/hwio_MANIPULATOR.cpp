//! @file
//! @brief hardware input output abstraction for Buddy board raped to be used as Manipulator
//!

#include "hwio.h"
#include <inttypes.h>
#include "config.h"
#include "stm32f4xx_hal.h"
#include "cmsis_os.h"
#include "gpio.h"
#include "adc.h"
#include "sim_nozzle.h"
#include "sim_bed.h"
#include "sim_motion.h"
#include "Arduino.h"
#include "timer_defaults.h"
#include "hwio_pindef.h"
#include "filament_sensor.h"
#include "bsod.h"
#include "main.h"

template <class T, size_t N>
constexpr int num_elements(T (&)[N]) { return N; }

//hwio arduino wrapper errors
enum class HwioErr : uint_least8_t {
    UNINI_DIG_RD,
    UNINI_DIG_WR,
    UNINI_ANA_RD,
    UNINI_ANA_WR,
    UNDEF_DIG_RD,
    UNDEF_DIG_WR,
    UNDEF_ANA_RD,
    UNDEF_ANA_WR,
};

namespace {
// a3ides digital input pins
enum digIn {
    ePIN_Z_MAX = PIN_Z_MAX,
    ePIN_X_MAX = PIN_X_MAX,
    ePIN_E_DIAG = PIN_E_DIAG,
    ePIN_Y_DIAG = PIN_Y_DIAG,
    ePIN_X_DIAG = PIN_X_DIAG,
    ePIN_Z_DIAG = PIN_Z_DIAG,
    ePIN_BTN_ENC = PIN_BTN_ENC,
    ePIN_BTN_EN1 = PIN_BTN_ENC,
    ePIN_BTN_EN2 = PIN_BTN_ENC,
};

// a3ides digital output pins
enum digOut {
    ePIN_X_DIR = PIN_X_DIR,
    ePIN_X_STEP = PIN_X_STEP,
    ePIN_Z_ENABLE = PIN_Z_ENABLE,
    ePIN_X_ENABLE = PIN_X_ENABLE,
    ePIN_Z_STEP = PIN_Z_STEP,
    ePIN_E_DIR = PIN_E_DIR,
    ePIN_E_STEP = PIN_E_STEP,
    ePIN_E_ENABLE = PIN_E_ENABLE,
    ePIN_Y_DIR = PIN_Y_DIR,
    ePIN_Y_STEP = PIN_Y_STEP,
    ePIN_Y_ENABLE = PIN_Y_ENABLE,
    ePIN_Z_DIR = PIN_Z_DIR,
};

} //end anonymous namespace

//sampled analog inputs
int _adc_val[ONE_BEHIND_LAST_ADC] = { 0, 0, 0, 0, 0 };

#define _FAN_ID_MIN HWIO_PWM_FAN1
#define _FAN_ID_MAX HWIO_PWM_FAN
#define _FAN_CNT    (_FAN_ID_MAX - _FAN_ID_MIN + 1)

#define _HEATER_ID_MIN HWIO_PWM_HEATER_BED
#define _HEATER_ID_MAX HWIO_PWM_HEATER_0
#define _HEATER_CNT    (_HEATER_ID_MAX - _HEATER_ID_MIN + 1)

//this value is compared to new value (to avoid rounding errors)
static int _tim1_period_us = GEN_PERIOD_US(TIM1_default_Prescaler, TIM1_default_Period);
static int _tim3_period_us = GEN_PERIOD_US(TIM3_default_Prescaler, TIM3_default_Period);

// a3ides pwm output pins
static const uint32_t _pwm_pin32[] = {
    PIN_HEATER_0,
    PIN_HEATER_BED,
    PIN_FAN1,
    PIN_FAN
};

static const uint32_t _pwm_chan[] = {
    TIM_CHANNEL_3, //_PWM_HEATER_BED
    TIM_CHANNEL_4, //_PWM_HEATER_0
    TIM_CHANNEL_1, //_PWM_FAN1
    TIM_CHANNEL_2, //_PWM_FAN
};

static TIM_HandleTypeDef *_pwm_p_htim[] = {
    &htim3, //_PWM_HEATER_BED
    &htim3, //_PWM_HEATER_0
    &htim1, //_PWM_FAN1
    &htim1, //_PWM_FAN
};

static int *const _pwm_period_us[] = {
    &_tim3_period_us, //_PWM_HEATER_BED
    &_tim3_period_us, //_PWM_HEATER_0
    &_tim1_period_us, //_PWM_FAN1
    &_tim1_period_us, //_PWM_FAN
};

// a3ides pwm output maximum values
static const int _pwm_max[] = { TIM3_default_Period, TIM3_default_Period, TIM1_default_Period, TIM1_default_Period }; //{42000, 42000, 42000, 42000};
constexpr int _PWM_CNT = (sizeof(_pwm_pin32) / sizeof(_pwm_pin32[0]));

static const TIM_OC_InitTypeDef sConfigOC_default = {
    TIM_OCMODE_PWM1,       //OCMode
    0,                     //Pulse
    TIM_OCPOLARITY_HIGH,   //OCPolarity
    TIM_OCNPOLARITY_HIGH,  //OCNPolarity
    TIM_OCFAST_DISABLE,    //OCFastMode
    TIM_OCIDLESTATE_RESET, //OCIdleState
    TIM_OCNIDLESTATE_RESET //OCNIdleState
};

// a3ides pwm output maximum values  as arduino analogWrite
static const int _pwm_analogWrite_max[_PWM_CNT] = { 0xff, 0xff, 0xff, 0xff };
// a3ides fan output values  as arduino analogWrite
static int _pwm_analogWrite_val[_PWM_CNT] = { 0, 0, 0, 0 };

/*****************************************************************************
 * private function declarations
 * */
static void __pwm_set_val(TIM_HandleTypeDef *htim, uint32_t chan, int val);
static void _hwio_pwm_analogWrite_set_val(int i_pwm, int val);
static void _hwio_pwm_set_val(int i_pwm, int val);
static int _pwm_get_chan(int i_pwm);
static TIM_HandleTypeDef *_pwm_get_htim(int i_pwm);
static int is_pwm_id_valid(int i_pwm);

//--------------------------------------
//analog input functions

int hwio_adc_get_val(ADC_t adc) //read analog input
{
    int i_adc = static_cast<int>(adc);
    if ((i_adc >= 0) && (i_adc < num_elements(_adc_val)))
        return _adc_val[i_adc];
    //else //TODO: check
    return 0;
}

//--------------------------------------
//pwm output functions

int is_pwm_id_valid(int i_pwm) {
    return ((i_pwm >= 0) && (i_pwm < _PWM_CNT));
}

int hwio_pwm_get_max(int i_pwm) //pwm output maximum value
{
    if (!is_pwm_id_valid(i_pwm))
        return -1;
    return _pwm_max[i_pwm];
}

//values should be:
//0000 0000 0000 0000 -exp = 0
//0000 0000 0000 0001 -exp = 1
//0000 0000 0000 0011 -exp = 2
//0000 0000 0000 0111 -exp = 3
//..
//0111 1111 1111 1111 -exp = 15
//1111 1111 1111 1111 -exp = 16

//reading value set by hwio_pwm_set_prescaler_exp2

int _pwm_get_chan(int i_pwm) {
    if (!is_pwm_id_valid(i_pwm))
        return -1;
    return _pwm_chan[i_pwm];
}

TIM_HandleTypeDef *_pwm_get_htim(int i_pwm) {
    if (!is_pwm_id_valid(i_pwm))
        i_pwm = 0;

    return _pwm_p_htim[i_pwm];
}

void hwio_pwm_set_val(int i_pwm, uint32_t val) //write pwm output and actualize _pwm_analogWrite_val
{
    if (!is_pwm_id_valid(i_pwm))
        return;

    uint32_t chan = _pwm_get_chan(i_pwm);
    TIM_HandleTypeDef *htim = _pwm_get_htim(i_pwm);
    uint32_t cmp = __HAL_TIM_GET_COMPARE(htim, chan);

    if ((_pwm_analogWrite_val[i_pwm] ^ val) || (cmp != val)) {
        _hwio_pwm_set_val(i_pwm, val);

        //actualize _pwm_analogWrite_val
        int pwm_max = hwio_pwm_get_max(i_pwm);
        int pwm_analogWrite_max = _pwm_analogWrite_max[i_pwm];

        uint32_t pulse = (val * pwm_analogWrite_max) / pwm_max;
        _pwm_analogWrite_val[i_pwm] = pulse; //arduino compatible
    }
}

void _hwio_pwm_set_val(int i_pwm, int val) //write pwm output
{
    int chan = _pwm_get_chan(i_pwm);
    TIM_HandleTypeDef *htim = _pwm_get_htim(i_pwm);
    if ((chan == -1) || htim->Instance == 0) {
        return;
    }

    __pwm_set_val(htim, chan, val);
}

void __pwm_set_val(TIM_HandleTypeDef *htim, uint32_t pchan, int val) //write pwm output
{
    if (htim->Init.Period) {
        TIM_OC_InitTypeDef sConfigOC = sConfigOC_default;
        if (val)
            sConfigOC.Pulse = val;
        else {
            sConfigOC.Pulse = htim->Init.Period;
            if (sConfigOC.OCPolarity == TIM_OCPOLARITY_HIGH)
                sConfigOC.OCPolarity = TIM_OCPOLARITY_LOW;
            else
                sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
        }
        if (HAL_TIM_PWM_ConfigChannel(htim, &sConfigOC, pchan) != HAL_OK) {
            Error_Handler();
        }
        HAL_TIM_PWM_Start(htim, pchan);
    } else
        HAL_TIM_PWM_Stop(htim, pchan);
}

void _hwio_pwm_analogWrite_set_val(int i_pwm, int val) {
    if (!is_pwm_id_valid(i_pwm))
        return;

    if (_pwm_analogWrite_val[i_pwm] != val) {
        int32_t pwm_max = hwio_pwm_get_max(i_pwm);
        uint32_t pulse = (val * pwm_max) / _pwm_analogWrite_max[i_pwm];
        hwio_pwm_set_val(i_pwm, pulse);
        _pwm_analogWrite_val[i_pwm] = val;
    }
}

//--------------------------------------
//fan control functions

int hwio_fan_get_cnt(void) //number of fans
{ return _FAN_CNT; }

void hwio_fan_set_pwm(int i_fan, int val) {
    i_fan += _FAN_ID_MIN;
    if ((i_fan >= _FAN_ID_MIN) && (i_fan <= _FAN_ID_MAX))
        _hwio_pwm_analogWrite_set_val(i_fan, val);
}

//--------------------------------------
// heater control functions

int hwio_heater_get_cnt(void) //number of heaters
{ return _HEATER_CNT; }

void hwio_heater_set_pwm(int i_heater, int val) {
    i_heater += _HEATER_ID_MIN;
    if ((i_heater >= _HEATER_ID_MIN) && (i_heater <= _HEATER_ID_MAX))
        _hwio_pwm_analogWrite_set_val(i_heater, val);
}

//--------------------------------------
// Beeper

void hwio_beeper_set_vol(float vol) {}

void hwio_beeper_set_pwm(uint32_t, uint32_t) {}

void hwio_beeper_tone(float, uint32_t) {}

void hwio_beeper_tone2(float, uint32_t, float) {}

void hwio_beeper_notone(void) {}

void hwio_update_1ms(void) {}

//--------------------------------------
// ADC sampler

// value ready callback
void adc_ready(uint8_t index) {
    _adc_val[index] = adc_val[index] >> 4;
}

// channel priority sequence callback
const uint8_t adc_seq[18] = { 4, 1, 4, 1, 4, 0, 4, 1, 4, 1, 4, 2, 4, 1, 4, 1, 4, 3 };
uint8_t adc_seq2idx(uint8_t seq) {
    return adc_seq[seq];
    //	return 2;
    //	if ((seq % 3) != 2) return 2;
    //	if (seq != 8) return 0;
    //	return 1;
}

//--------------------------------------
// Arduino digital/analog read/write error handler

void hwio_arduino_error(HwioErr err, uint32_t pin32) {
    char text[64];
    if ((err == HwioErr::UNINI_DIG_WR) && (pin32 == PIN_BEEPER))
        return; //ignore BEEPER write
    strcat(text, "HWIO error\n");
    switch (err) {
    case HwioErr::UNINI_DIG_RD:
    case HwioErr::UNINI_DIG_WR:
    case HwioErr::UNINI_ANA_RD:
    case HwioErr::UNINI_ANA_WR:
        strcat(text, "uninitialized\n");
        break;
    case HwioErr::UNDEF_DIG_RD:
    case HwioErr::UNDEF_DIG_WR:
    case HwioErr::UNDEF_ANA_RD:
    case HwioErr::UNDEF_ANA_WR:
        strcat(text, "undefined\n");
        break;
    }
    sprintf(text + strlen(text), "pin #%u (0x%02x)\n", (int)pin32, (uint8_t)pin32);
    switch (err) {
    case HwioErr::UNINI_DIG_RD:
    case HwioErr::UNINI_DIG_WR:
    case HwioErr::UNDEF_DIG_RD:
    case HwioErr::UNDEF_DIG_WR:
        strcat(text, "digital ");
        break;
    case HwioErr::UNINI_ANA_RD:
    case HwioErr::UNINI_ANA_WR:
    case HwioErr::UNDEF_ANA_RD:
    case HwioErr::UNDEF_ANA_WR:
        strcat(text, "analog ");
        break;
    }
    switch (err) {
    case HwioErr::UNINI_DIG_RD:
    case HwioErr::UNDEF_DIG_RD:
    case HwioErr::UNINI_ANA_RD:
    case HwioErr::UNDEF_ANA_RD:
        strcat(text, "read");
        break;
    case HwioErr::UNINI_DIG_WR:
    case HwioErr::UNDEF_DIG_WR:
    case HwioErr::UNINI_ANA_WR:
    case HwioErr::UNDEF_ANA_WR:
        strcat(text, "write");
        break;
    }
    bsod(text);
}

//--------------------------------------
// Arduino digital/analog wrapper functions

int hwio_arduino_digitalRead(uint32_t ulPin) {
    if (HAL_GPIO_Initialized) {
        switch (ulPin) {
#ifdef SIM_MOTION
        case PIN_Z_MIN:
            return sim_motion_get_min_end(2);
        case PIN_E_DIAG:
            return sim_motion_get_diag(3);
        case PIN_Y_DIAG:
            return sim_motion_get_diag(1);
        case PIN_X_DIAG:
            return sim_motion_get_diag(0);
        case PIN_Z_DIAG:
            return sim_motion_get_diag(2);
#else  //SIM_MOTION
        case PIN_Z_MAX:
            return gpio_get(digIn::ePIN_Z_MAX);
        case PIN_X_MAX:
            return gpio_get(digIn::ePIN_X_MAX);
        case PIN_E_DIAG:
            return gpio_get(digIn::ePIN_E_DIAG);
        case PIN_Y_DIAG:
            return gpio_get(digIn::ePIN_Y_DIAG);
        case PIN_X_DIAG:
            return gpio_get(digIn::ePIN_X_DIAG);
        case PIN_Z_DIAG:
            return gpio_get(digIn::ePIN_Z_DIAG);
#endif //SIM_MOTION
        case PIN_BTN_ENC:
            return gpio_get(digIn::ePIN_BTN_ENC);
        case PIN_BTN_EN1:
            return gpio_get(digIn::ePIN_BTN_EN1);
        case PIN_BTN_EN2:
            return gpio_get(digIn::ePIN_BTN_EN2);
        default:
            hwio_arduino_error(HwioErr::UNDEF_DIG_RD, ulPin); //error: undefined pin digital read
        }
    } else
        hwio_arduino_error(HwioErr::UNINI_DIG_RD, ulPin); //error: uninitialized digital read
    return 0;
}

void hwio_arduino_digitalWrite(uint32_t ulPin, uint32_t ulVal) {
    if (HAL_GPIO_Initialized) {
        switch (ulPin) {
        case PIN_BEEPER:
            return;
        case PIN_HEATER_BED:
#ifdef SIM_HEATER_BED_ADC
            if (adc_sim_msk & (1 << SIM_HEATER_BED_ADC))
                sim_bed_set_power(ulVal ? 100 : 0);
            else
#endif //SIM_HEATER_BED_ADC
                _hwio_pwm_analogWrite_set_val(HWIO_PWM_HEATER_BED, ulVal ? _pwm_analogWrite_max[HWIO_PWM_HEATER_BED] : 0);
            return;
        case PIN_HEATER_0:
#ifdef SIM_HEATER_NOZZLE_ADC
            if (adc_sim_msk & (1 << SIM_HEATER_NOZZLE_ADC))
                sim_nozzle_set_power(ulVal ? 40 : 0);
            else
#endif //SIM_HEATER_NOZZLE_ADC
                _hwio_pwm_analogWrite_set_val(HWIO_PWM_HEATER_0, ulVal ? _pwm_analogWrite_max[HWIO_PWM_HEATER_0] : 0);
            return;
        case PIN_FAN1:
            //_hwio_pwm_analogWrite_set_val(HWIO_PWM_FAN1, ulVal ? _pwm_analogWrite_max[HWIO_PWM_FAN1] : 0);
#ifdef PRINTER_PRUSA_MK4
            _hwio_pwm_analogWrite_set_val(HWIO_PWM_FAN1, ulVal ? 200 : 0);
#else
            _hwio_pwm_analogWrite_set_val(HWIO_PWM_FAN1, ulVal ? 100 : 0);
#endif
            return;
        case PIN_FAN:
            _hwio_pwm_analogWrite_set_val(HWIO_PWM_FAN, ulVal ? _pwm_analogWrite_max[HWIO_PWM_FAN] : 0);
            return;
#ifdef SIM_MOTION
        case PIN_X_DIR:
            sim_motion_set_dir(0, ulVal ? 1 : 0);
            return;
        case PIN_X_STEP:
            sim_motion_set_stp(0, ulVal ? 1 : 0);
            return;
        case PIN_Z_ENABLE:
            sim_motion_set_ena(2, ulVal ? 1 : 0);
            return;
        case PIN_X_ENABLE:
            sim_motion_set_ena(0, ulVal ? 1 : 0);
            return;
        case PIN_Z_STEP:
            sim_motion_set_stp(2, ulVal ? 1 : 0);
            return;
        case PIN_E_DIR:
            sim_motion_set_dir(3, ulVal ? 1 : 0);
            return;
        case PIN_E_STEP:
            sim_motion_set_stp(3, ulVal ? 1 : 0);
            return;
        case PIN_E_ENABLE:
            sim_motion_set_ena(3, ulVal ? 1 : 0);
            return;
        case PIN_Y_DIR:
            sim_motion_set_dir(1, ulVal ? 1 : 0);
            return;
        case PIN_Y_STEP:
            sim_motion_set_stp(1, ulVal ? 1 : 0);
            return;
        case PIN_Y_ENABLE:
            sim_motion_set_ena(1, ulVal ? 1 : 0);
            return;
        case PIN_Z_DIR:
            sim_motion_set_dir(2, ulVal ? 1 : 0);
            return;
#else  //SIM_MOTION
        case PIN_X_DIR:
            gpio_set(digOut::ePIN_X_DIR, ulVal ? 1 : 0);
            return;
        case PIN_X_STEP:
            gpio_set(digOut::ePIN_X_STEP, ulVal ? 1 : 0);
            return;
        case PIN_Z_ENABLE:
            gpio_set(digOut::ePIN_Z_ENABLE, ulVal ? 1 : 0);
            return;
        case PIN_X_ENABLE:
            gpio_set(digOut::ePIN_X_ENABLE, ulVal ? 1 : 0);
            return;
        case PIN_Z_STEP:
            gpio_set(digOut::ePIN_Z_STEP, ulVal ? 1 : 0);
            return;
        case PIN_E_DIR:
            gpio_set(digOut::ePIN_E_DIR, ulVal ? 1 : 0);
            return;
        case PIN_E_STEP:
            gpio_set(digOut::ePIN_E_STEP, ulVal ? 1 : 0);
            return;
        case PIN_E_ENABLE:
            gpio_set(digOut::ePIN_E_ENABLE, ulVal ? 1 : 0);
            return;
        case PIN_Y_DIR:
            gpio_set(digOut::ePIN_Y_DIR, ulVal ? 1 : 0);
            return;
        case PIN_Y_STEP:
            gpio_set(digOut::ePIN_Y_STEP, ulVal ? 1 : 0);
            return;
        case PIN_Y_ENABLE:
            gpio_set(digOut::ePIN_Y_ENABLE, ulVal ? 1 : 0);
            return;
        case PIN_Z_DIR:
            gpio_set(digOut::ePIN_Z_DIR, ulVal ? 1 : 0);
            return;
#endif //SIM_MOTION
        default:
            hwio_arduino_error(HwioErr::UNDEF_DIG_WR, ulPin); //error: undefined pin digital write
        }
    } else
        hwio_arduino_error(HwioErr::UNINI_DIG_WR, ulPin); //error: uninitialized digital write
}

void hwio_arduino_digitalToggle(uint32_t ulPin) {
    hwio_arduino_digitalWrite(ulPin, !hwio_arduino_digitalRead(ulPin));
    // TODO test me
}

uint32_t hwio_arduino_analogRead(uint32_t ulPin) {
    if (HAL_ADC_Initialized) {
        switch (ulPin) {
        case PIN_TEMP_BED:
            return hwio_adc_get_val(ADC_t::ADC_TEMP_BED);
        case PIN_TEMP_0:
            return hwio_adc_get_val(ADC_t::ADC_TEMP_0);
        case PIN_TEMP_HEATBREAK:
            return hwio_adc_get_val(ADC_t::ADC_TEMP_HEATBREAK);
        default:
            hwio_arduino_error(HwioErr::UNDEF_ANA_RD, ulPin); //error: undefined pin analog read
        }
    } else
        hwio_arduino_error(HwioErr::UNINI_ANA_RD, ulPin); //error: uninitialized analog read
    return 0;
}

void hwio_arduino_analogWrite(uint32_t ulPin, uint32_t ulValue) {
    if (HAL_PWM_Initialized) {
        switch (ulPin) {
        case PIN_FAN1:
            //hwio_fan_set_pwm(_FAN1, ulValue);
            _hwio_pwm_analogWrite_set_val(HWIO_PWM_FAN1, ulValue);
            return;
        case PIN_FAN:
            _hwio_pwm_analogWrite_set_val(HWIO_PWM_FAN, ulValue);
            return;
        case PIN_HEATER_BED:
            _hwio_pwm_analogWrite_set_val(HWIO_PWM_HEATER_BED, ulValue);
            return;
        case PIN_HEATER_0:
            _hwio_pwm_analogWrite_set_val(HWIO_PWM_HEATER_0, ulValue);
            return;
        default:
            hwio_arduino_error(HwioErr::UNDEF_ANA_WR, ulPin); //error: undefined pin analog write
        }
    } else
        hwio_arduino_error(HwioErr::UNINI_ANA_WR, ulPin); //error: uninitialized analog write
}

void hwio_arduino_pinMode(uint32_t ulPin, uint32_t ulMode) {
    // not supported, all pins are configured with Cube
}
