// adc.h
#ifndef _ADC_H
#define _ADC_H

#include <inttypes.h>
#include "config.h"
#include "config_a3ides2209_02.h"

#ifdef __cplusplus
extern "C" {
#endif //__cplusplus

#ifdef ADC_EXT_MUX
enum {
    MUX_CHANNEL_A,
    MUX_CHANNEL_B,
} typedef EXT_mux_channel_t;

enum {
    BED_VOLTAGE = 0,
    HEATER_VOLTAGE = 1,
    THERM_2 = 2,
    RESET_OVERCURRENT = 3,
    HEATER_CURRENT = 0,
    INPUT_CURRENT = 1,
    HEATER_FAULT = 2,
    CURRENT_FAULT = 3
} typedef EXT_mux_select_t;
#endif

extern uint32_t adc_val[ADC_CHAN_CNT];

extern uint32_t adc_sim_val[ADC_CHAN_CNT];

extern uint32_t adc_sim_msk;

extern void adc_init(void);

extern void adc_cycle(void);
extern void adc2_cycle();

#ifdef ADC_EXT_MUX
extern void adc_reset_fault_signal(void);

extern int adc_ext_mux_get_raw(EXT_mux_channel_t ext_mux_channel, EXT_mux_select_t ext_mux_select);
#endif

#ifdef __cplusplus
}
#endif //__cplusplus

#endif // _ADC_H
