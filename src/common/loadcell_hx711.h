// loadcell_hx711.h
#ifndef _LOADCELL_H
#define _LOADCELL_H

#include <inttypes.h>
#include "config.h"

#ifdef __cplusplus
extern "C" {
#endif //__cplusplus

extern int32_t loadcell_value;    // raw value readed from ADC
extern float loadcell_scale;      // scale (calibration value grams/raw)
extern float loadcell_load;       // load in grams
extern float loadcell_threshold;  // threshold for probe in grams
extern float loadcell_hysteresis; // hysteresis for probe in grams
extern int loadcell_probe;        // probe status
extern int loadcell_error;        // error counter
extern int loadcell_count;        // cycle counter

#ifdef FILAMENT_SENSOR_HX711

typedef enum {
    HX711_has_filament,
    HX711_no_filament,
    HX711_disconnected
} HX711_t;

extern int32_t fsensor_value;
extern HX711_t fsensor_probe;
extern int32_t fsensor_threshold_LO;
extern int32_t fsensor_threshold_HI;

// handle masking filament sensor
extern void set_fsensor_disable(uint8_t state);

#endif

// init - gpio setup and global variables
extern void loadcell_init(uint8_t pin_dout, uint8_t pin_sck);

// callback function called from timer interrupt - performs sampling and load calculation
extern void hx711_cycle(uint8_t cycle);

// offset adjustment
extern void loadcell_tare(void);

#ifdef __cplusplus
}
#endif //__cplusplus

#endif // _LOADCELL_H
