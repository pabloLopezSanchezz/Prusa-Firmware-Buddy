// loadcell_hx711.c
#include "loadcell_hx711.h"

#ifdef LOADCELL_HX711

    #include "gpio.h"
    #include "dbg.h"
    #include "cmsis_os.h"

    #define _CH_A_GAIN_128 1
    #define _CH_A_GAIN_64  3
    #define _CH_B_GAIN_32  2

    #ifdef HX711_ESP_DEBUG
        // ESP debug output to scope
        #define LOADCELL_DBG_ESPGPIO0_PIN PE6  // CLOCK Duplicate
        #define LOADCELL_DBG_ESPRX_PIN    PC6  // hx711_cycle(2); // Return data for channel A, and read filament sensor data data
        #define LOADCELL_DBG_ESPTX_PIN    PC7  // hx711_cycle(1); // Set data for channel B, and read load cell data
        #define LOADCELL_DBG_ESPRST_PIN   PC13 // hx711_cycle(0); // Set data for channel A, and read load cell data
    #endif

// HX711 variables
uint8_t hx711_pin_dout = 0; // DOUT pin
uint8_t hx711_pin_sck = 0;  // SCK pin
uint8_t hx711_config = 0;   // gain\channel, value 0 means not initialized

// Loadcell variables
int32_t loadcell_offset = 0;
int32_t loadcell_value = 0;
float loadcell_scale = 0;
float loadcell_load = 0;
float loadcell_threshold = 0;
float loadcell_hysteresis = 0;
int loadcell_probe = 0;
int loadcell_error = 0;
int loadcell_count = 0;

    #ifdef FILAMENT_SENSOR_HX711

uint8_t previous_cycle = 0;
uint8_t fsensor_disable = 0;

int32_t fsensor_value = 0;
HX711_t fsensor_probe = HX711_has_filament;
int32_t fsensor_threshold_LO = FILAMENT_SENSOR_HX711_LOW;
int32_t fsensor_threshold_HI = FILAMENT_SENSOR_HX711_HIGH;

    #endif

static inline void hx711_init(void) {
    gpio_init(hx711_pin_sck, GPIO_MODE_OUTPUT_PP, GPIO_NOPULL, GPIO_SPEED_FREQ_HIGH);
    gpio_init(hx711_pin_dout, GPIO_MODE_INPUT, GPIO_PULLUP, GPIO_SPEED_FREQ_VERY_HIGH);
    gpio_set(hx711_pin_sck, 0);

    #ifdef HX711_ESP_DEBUG
    // ESP debug output to scope
    gpio_init(LOADCELL_DBG_ESPGPIO0_PIN, GPIO_MODE_OUTPUT_PP, GPIO_NOPULL, GPIO_SPEED_FREQ_HIGH);
    gpio_init(LOADCELL_DBG_ESPRX_PIN, GPIO_MODE_OUTPUT_PP, GPIO_NOPULL, GPIO_SPEED_FREQ_HIGH);
    gpio_init(LOADCELL_DBG_ESPTX_PIN, GPIO_MODE_OUTPUT_PP, GPIO_NOPULL, GPIO_SPEED_FREQ_HIGH);
    gpio_init(LOADCELL_DBG_ESPRST_PIN, GPIO_MODE_OUTPUT_PP, GPIO_NOPULL, GPIO_SPEED_FREQ_HIGH);

    gpio_set(LOADCELL_DBG_ESPGPIO0_PIN, 0);
    gpio_set(LOADCELL_DBG_ESPRX_PIN, 0);
    gpio_set(LOADCELL_DBG_ESPRST_PIN, 0);
    #endif

    #ifdef LOADCELL_LATENCY_TEST
    gpio_init(PC13, GPIO_MODE_OUTPUT_PP, GPIO_NOPULL, GPIO_SPEED_FREQ_HIGH);
    gpio_set(PC13, 0);
    #endif //LOADCELL_LATENCY_TEST
}

// init - gpio setup and global variables
void loadcell_init(uint8_t pin_dout, uint8_t pin_sck) {
    hx711_pin_dout = pin_dout; // DOUT pin
    hx711_pin_sck = pin_sck;   // SCK pin
    hx711_init();
    loadcell_scale = 0.0100;       // default scale [g/1]
    loadcell_threshold = -40;      // default threshold [g]
    loadcell_hysteresis = 20;      // default hysteresis [g]
    hx711_config = _CH_A_GAIN_128; // set config != 0, start measurement
}

static inline uint8_t hx711_ready(void) {
    return gpio_get(hx711_pin_dout) ? 0 : 1;
}

// sample raw bits from ADC
static inline int32_t hx711_read_raw(void) {
    int index;
    int32_t result = 0;
    // read 24-bit from HX711
    for (index = 0; index < 24; index++) {
        gpio_set(hx711_pin_sck, 1);
        //gpio_set(LOADCELL_DBG_ESPGPIO0_PIN, 1);
        gpio_set(hx711_pin_sck, 1);
        //gpio_set(LOADCELL_DBG_ESPGPIO0_PIN, 1);
        result = (result << 1) | gpio_get(hx711_pin_dout);
        gpio_set(hx711_pin_sck, 0);
        //gpio_set(LOADCELL_DBG_ESPGPIO0_PIN, 0);
    }
    // set measure mode
    for (index = 0; index < hx711_config; index++) {
        gpio_set(hx711_pin_sck, 1);
        //gpio_set(LOADCELL_DBG_ESPGPIO0_PIN, 1);
        gpio_set(hx711_pin_sck, 1);
        //gpio_set(LOADCELL_DBG_ESPGPIO0_PIN, 1);
        gpio_set(hx711_pin_sck, 0);
        //gpio_set(LOADCELL_DBG_ESPGPIO0_PIN, 0);
    }
    // convert 24-bit to double-supplement number on 32-bit long number
    return (result >= 0x800000) ? (result | 0xFF000000) : result;
}

/*
This was causing trouble, hx711_read(); does multiply currently measured value, which does not represent reality, DO NOT USE!

// sample and recalculate to max resolution (2.384185791 nanoVolt)
static inline int32_t hx711_read(void)
{
	int multiply = 0;
	// multiply factor for selected gain
	switch (hx711_config)
	{
	case 1: multiply = 1; break;
	case 2: multiply = 4; break;
	case 3: multiply = 2; break;
	}
	return (multiply)?(multiply * hx711_read_raw()):0;
}
*/

    #include "metric.h"

metric_t metric_loadcell_raw = METRIC("loadcell_raw", METRIC_VALUE_INTEGER, 0, METRIC_HANDLER_DISABLE_ALL);
metric_t metric_loadcell = METRIC("loadcell", METRIC_VALUE_FLOAT, 0, METRIC_HANDLER_DISABLE_ALL);
metric_t metric_fsensor_raw = METRIC("fsensor_raw", METRIC_VALUE_INTEGER, 0, METRIC_HANDLER_DISABLE_ALL);

static inline void loadcell_cycle(void) {
    #ifdef HX711_ESP_DEBUG
    gpio_set(LOADCELL_DBG_ESPRX_PIN, 1);
    #endif
    float load;
    // sample
    loadcell_value = hx711_read_raw();
    metric_record_integer(&metric_loadcell_raw, loadcell_value);

    // scale to grams
    load = loadcell_scale * (loadcell_value - loadcell_offset);
    metric_record_float(&metric_loadcell, load);
    // update probe variable
    if (loadcell_probe) {
        if (load >= (loadcell_threshold + loadcell_hysteresis))
            loadcell_probe = 0;
    } else {
        if (load <= loadcell_threshold) {
            loadcell_probe = 1;
    #ifdef LOADCELL_LATENCY_TEST
            gpio_set(PC13, 1);
    #endif //LOADCELL_LATENCY_TEST
        }
    }
    loadcell_load = load;

    loadcell_count++;
}

    #ifdef FILAMENT_SENSOR_HX711
static inline void fsensor_cycle(void) {
        #ifdef HX711_ESP_DEBUG
    gpio_set(LOADCELL_DBG_ESPTX_PIN, 1);
        #endif
    fsensor_value = hx711_read_raw();
    metric_record_integer(&metric_fsensor_raw, fsensor_value);
    // update probe variable
    if (fsensor_value <= (fsensor_threshold_LO)) {
        fsensor_probe = HX711_has_filament;
    } else if (fsensor_value < 2000) {
        fsensor_probe = HX711_disconnected;
    }

    else {
        fsensor_probe = HX711_no_filament;
    }
}

// Public function to handle masking filament sensor
void set_fsensor_disable(uint8_t state) {
    if (state == 1) {
        fsensor_disable = 1;
        #ifdef HX711_ESP_DEBUG
        gpio_set(LOADCELL_DBG_ESPGPIO0_PIN, 1);
        #endif
    } else {
        fsensor_disable = 0;
        #ifdef HX711_ESP_DEBUG
        gpio_set(LOADCELL_DBG_ESPGPIO0_PIN, 0);
        #endif
    }
}

    #endif

// callback function called from timer interrupt - performs sampling and load calculation
void hx711_cycle(uint8_t cycle) {
    #ifdef HX711_ESP_DEBUG
    gpio_set(LOADCELL_DBG_ESPRX_PIN, 0);
    gpio_set(LOADCELL_DBG_ESPTX_PIN, 0);
    //gpio_set(LOADCELL_DBG_ESPRST_PIN, 1);
    #endif

    while (!hx711_ready()) {
        //gpio_set(LOADCELL_DBG_ESPRX_PIN, 1);
    }

    if (hx711_config == 0) // not configured, skip measurement
        return;            // HX711 not configured properly
    if (hx711_ready())     // ready, do measurement
    {

    #ifdef FILAMENT_SENSOR_HX711
        if (fsensor_disable == 1) {
            // If filament sensor is explicitly disable for example beacuse of bed leveling
            hx711_config = _CH_A_GAIN_128;

            // check previous hazard state
            if (previous_cycle == 1) {
                fsensor_cycle();
                previous_cycle = 0;
            } else {
                loadcell_cycle();
            }

        } else {
            switch (cycle) {
            case 0:
                // Set data for channel A, and read load cell data, normal operation
                if ((previous_cycle == 2) || (previous_cycle == 0)) // Check previous state
                {
    #endif
                    hx711_config = _CH_A_GAIN_128;
                    loadcell_cycle();

    #ifdef FILAMENT_SENSOR_HX711
                }
                break;
            case 1:
                // Set data for channel B, and read load cell data
                if (previous_cycle == 0) // Check previous state
                {
                    hx711_config = _CH_B_GAIN_32;
                    loadcell_cycle();
                }
                break;
            case 2:
                // Return data for channel A, and read filament sensor data data
                if (previous_cycle == 1) // Check previous state
                {
                    hx711_config = _CH_A_GAIN_128;
                    fsensor_cycle();
                }
                break;
            }
            previous_cycle = cycle;
        }
    #endif
        //return; // Cycle OK
    } else {
        loadcell_error++;

        //return; // Cycle fail, HX711 not ready, try again later
    }

    //gpio_set(LOADCELL_DBG_ESPRST_PIN, 0);
}

    #define _TARE_SAMPLES 4

// offset adjustment
void loadcell_tare(void) {
    #ifdef FILAMENT_SENSOR_HX711
    uint8_t was_disabled = fsensor_disable;
    if (fsensor_disable == 0) {
        set_fsensor_disable(1);
    }
    #endif
    int samples = _TARE_SAMPLES;
    int32_t sum = 0;
    int count;
    while (samples--) {
        count = loadcell_count;
        while (count == loadcell_count)
            osDelay(5);
        sum += loadcell_value;
        _dbg("TARE %d %d", (_TARE_SAMPLES - samples), loadcell_value);
    }
    loadcell_offset = sum / _TARE_SAMPLES;
    _dbg("TARE: %d", loadcell_value);
    loadcell_load = 0;
    loadcell_error = 0;
    #ifdef FILAMENT_SENSOR_HX711
    set_fsensor_disable(was_disabled);
    #endif
}

#endif //LOADCELL_HX711
