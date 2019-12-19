// gui_config_mini.h
#ifndef _GUI_CONFIG_MINI_H_
#define _GUI_CONFIG_MINI_H_

// axis length [mm] - PRINTER MINI
#define X_LEN 255
#define Y_LEN 212.5
#define Z_LEN 220

// tolerance (common for all axes)
#define LEN_TOL_ABS 15 // length absolute tolerance (+-5mm)
#define LEN_TOL_REV 13 // length tolerance in reversed direction (3mm)

//#define Z_OFFSET_STEP     0.0025F//calculated
#define Z_OFFSET_MIN -2.0F
#define Z_OFFSET_MAX 2.0F

#endif // _GUI_CONFIG_H_
