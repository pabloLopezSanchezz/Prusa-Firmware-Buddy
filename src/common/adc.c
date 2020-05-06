//adc.c
#include "adc.h"
#include "stm32f4xx_hal.h"
#include "hwio.h"
#include "ext_mux.h"
#include "gpio.h"
#include "stdbool.h"
#ifdef ADC_EXT_MUX
    #include "hwio_pindef.h"
#endif

#ifndef ADC_SIM_MSK
    #define ADC_SIM_MSK 0
#endif

extern ADC_HandleTypeDef hadc1;
extern ADC_HandleTypeDef hadc2;

//Main error handler
extern void Error_Handler(void);

uint32_t adc_val[ADC_CHAN_CNT];   //sampled values
uint32_t adc2_val[ADC2_CHAN_CNT]; //sampled values
uint8_t adc_cnt[ADC_CHAN_CNT];    //number of samples
uint8_t adc_chn[ADC_CHAN_CNT];    //physical channels
uint8_t adc_sta = 0xff;           //current state, 0xff means "not initialized"
int8_t adc_idx = 0;               //current value index

//external ADC variables
#ifdef ADC_EXT_MUX

bool adc2_initialized = false;

enum {
    FAULT_SIGNAL_RESET_IDLE,
    FAULT_SIGNAL_RESET_PENDING,
    FAULT_SIGNAL_RESET_WAITING_1MS,
    FAULT_SIGNAL_RESET_WAITING_2MS,
    FAULT_SIGNAL_RESET_WAITING_3MS,
    FAULT_SIGNAL_RESET_INIT_ADC
} typedef fault_signal_t;

enum {
    ADC2_SAMPLE_STATE_START,
    ADC2_SAMPLE_STATE_WAIT_FOR_EOC,
} typedef adc2_sample_state_t;

int _adc_raw_val_ext_mux[ADC_EXT_MUX_COMMON_CHAN_CNT][ADC_EXT_MUX_INDEPENDENT_CHAN_CNT] = {
    { 0, 0, 0, 0 }, // Channel A
    { 0, 0, 0, 0 }  // Channel B
};

uint8_t adc_ext_mux_mask = 0; // Mask for state machine

uint8_t adc_ext_mux_logic_input = 0; //represent now select ext mux pair

fault_signal_t fault_signal_state = FAULT_SIGNAL_RESET_IDLE;
adc2_sample_state_t adc2_sample_state = ADC2_SAMPLE_STATE_START;
uint8_t current_channel = ADC_CHANNEL_3;
uint8_t next_channel;
#endif

uint32_t adc_sim_val[ADC_CHAN_CNT]; //simulated values
uint32_t adc_sim_msk = ADC_SIM_MSK; //mask simulated channels

void adc_init_sim_vals(void);

//Reset overcurrent cycle call from adc2_cycle
#ifdef ADC_EXT_MUX
void adc_reset_fault_signal_cycle() {
    switch (fault_signal_state) {
    case FAULT_SIGNAL_RESET_IDLE:
        break;

    case FAULT_SIGNAL_RESET_PENDING:
        HAL_ADC_DeInit(&hadc2);
        adc2_initialized = false;
        gpio_init(PA3, GPIO_MODE_OUTPUT_PP, GPIO_NOPULL, GPIO_SPEED_FREQ_HIGH);
        ext_mux_select(3);
        gpio_set(PIN_EXT_MUX_CHANELL_A, 1);
        fault_signal_state = FAULT_SIGNAL_RESET_WAITING_1MS;
        return;

    case FAULT_SIGNAL_RESET_WAITING_1MS:
        fault_signal_state = FAULT_SIGNAL_RESET_WAITING_2MS;
        return;

    case FAULT_SIGNAL_RESET_WAITING_2MS:
        gpio_set(PIN_EXT_MUX_CHANELL_A, 0);
        fault_signal_state = FAULT_SIGNAL_RESET_WAITING_3MS;
        return;

    case FAULT_SIGNAL_RESET_WAITING_3MS:
        fault_signal_state = FAULT_SIGNAL_RESET_INIT_ADC;
        return;

    case FAULT_SIGNAL_RESET_INIT_ADC:
        HAL_ADC_Init(&hadc2);
        adc2_initialized = true;
        fault_signal_state = FAULT_SIGNAL_RESET_IDLE;
        break;
    }
}
#endif

//convert value index to physical channel
uint8_t adc_chan(uint8_t idx) {
    uint8_t chan = 0;
    uint16_t mask = 1;
    while (mask) {
        if ((mask & ADC_CHAN_MSK) && (idx-- == 0))
            break;
        mask <<= 1;
        chan++;
    }
    return chan;
}

//configure multiplexer
void adc_set_mux(uint8_t chn) {
    ADC_ChannelConfTypeDef sConfig = { 0 };
    sConfig.Channel = chn;
    sConfig.Rank = 1;
    sConfig.SamplingTime = ADC_SAMPLETIME_3CYCLES;
    if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK) {
        Error_Handler();
    }
}

void adc2_set_mux(uint8_t chn) {
    ADC_ChannelConfTypeDef sConfig = { 0 };
    sConfig.Channel = chn;
    sConfig.Rank = 1;
    sConfig.SamplingTime = ADC_SAMPLETIME_480CYCLES;
    if (HAL_ADC_ConfigChannel(&hadc2, &sConfig) != HAL_OK) {
        Error_Handler();
    }
}

//initialization
void adc_init(void) {
    for (int i = 0; i < ADC_CHAN_CNT; i++) {
        adc_val[i] = 0;
        adc_cnt[i] = 0;
        adc_chn[i] = adc_chan(i);
    }
    adc_idx = 0;
    adc_sta = 0x00;

    adc_init_sim_vals();
#ifdef ADC_EXT_MUX
    ext_mux_configure(EXT_MUX_SEL_A_PIN, EXT_MUX_SEL_B_PIN, ADC_MUX_CHANNEL_A, ADC_MUX_CHANNEL_B);
    adc2_initialized = true;
#endif
}

#ifdef ADC_EXT_MUX
int adc_ext_mux_get_raw(EXT_mux_channel_t ext_mux_channel, EXT_mux_select_t ext_mux_select) {
    return _adc_raw_val_ext_mux[ext_mux_channel][ext_mux_select];
}
#endif

//sampling cycle (called from interrupt)
#ifdef ADC_EXT_MUX
void adc2_cycle() {

    adc_reset_fault_signal_cycle();

    if (adc2_initialized) {
        switch (adc2_sample_state) {
        case ADC2_SAMPLE_STATE_START:
            adc2_set_mux(current_channel);
            HAL_ADC_Start(&hadc2);
            adc2_sample_state = ADC2_SAMPLE_STATE_WAIT_FOR_EOC;
            break;
        case ADC2_SAMPLE_STATE_WAIT_FOR_EOC:
            if (__HAL_ADC_GET_FLAG(&hadc2, ADC_FLAG_EOC)) {
                if (current_channel == ADC_CHANNEL_3) {
                    _adc_raw_val_ext_mux[0][adc_ext_mux_logic_input] = HAL_ADC_GetValue(&hadc2);
                    adc_ext_mux_mask |= 0x01;
                    next_channel = ADC_CHANNEL_5;
                    adc2_sample_state = ADC2_SAMPLE_STATE_START;
                } else {
                    _adc_raw_val_ext_mux[1][adc_ext_mux_logic_input] = HAL_ADC_GetValue(&hadc2);
                    adc_ext_mux_mask |= 0x02;
                    next_channel = ADC_CHANNEL_3;
                    adc2_sample_state = ADC2_SAMPLE_STATE_START;
                }
            }
            break;
        }

        if (adc_ext_mux_mask == 3) {
            adc_ext_mux_mask = 0;
            adc_ext_mux_logic_input++;

            if (adc_ext_mux_logic_input > 3) {
                adc_ext_mux_logic_input = 0;
            }
            ext_mux_select(adc_ext_mux_logic_input);
        }
        current_channel = next_channel;
    }
}
#endif

void adc_cycle(void) {
    uint8_t seq;
    uint32_t val;
    if (adc_sta == 0xff)
        return; //skip sampling if not initialized

    if (adc_sta & 0x80) {
        if (__HAL_ADC_GET_FLAG(&hadc1, ADC_FLAG_EOC)) {
            if ((1 << adc_idx) & adc_sim_msk)
                val = adc_sim_val[adc_idx];
            else
                val = HAL_ADC_GetValue(&hadc1);
            adc_val[adc_idx] += val;
            if (++adc_cnt[adc_idx] >= ADC_OVRSAMPL) {
                ADC_READY(adc_idx);
                adc_val[adc_idx] = 0;
                adc_cnt[adc_idx] = 0;
            }
            seq = (adc_sta & 0x1f) + 1;
            if (seq >= ADC_SEQ_LEN)
                seq = 0;
            adc_idx = ADC_SEQ2IDX(seq);
            adc_set_mux(adc_chn[adc_idx]);
            adc_sta = seq;
        } else {
            Error_Handler();
        }
    } else {
        HAL_ADC_Start(&hadc1);
        adc_sta |= 0x80;
    }
}

#ifdef ADC_EXT_MUX
void adc_reset_fault_signal(void) {
    fault_signal_state = FAULT_SIGNAL_RESET_PENDING;
}
#endif

void adc_init_sim_vals(void) {
#ifdef ADC_SIM_VAL0
    adc_sim_val[0] = ADC_SIM_VAL0;
#endif //ADC_SIM_VAL0
#ifdef ADC_SIM_VAL1
    adc_sim_val[1] = ADC_SIM_VAL1;
#endif //ADC_SIM_VAL1
#ifdef ADC_SIM_VAL2
    adc_sim_val[2] = ADC_SIM_VAL2;
#endif //ADC_SIM_VAL2
#ifdef ADC_SIM_VAL3
    adc_sim_val[3] = ADC_SIM_VAL3;
#endif //ADC_SIM_VAL3
#ifdef ADC_SIM_VAL4
    adc_sim_val[4] = ADC_SIM_VAL4;
#endif //ADC_SIM_VAL4
#ifdef ADC_SIM_VAL5
    adc_sim_val[5] = ADC_SIM_VAL5;
#endif //ADC_SIM_VAL4
}
