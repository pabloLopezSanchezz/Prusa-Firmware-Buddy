// display.c

#include "display.h"
#ifdef USE_ST7789
#include "st7789v.h"
#endif

#ifdef USE_ILI9488
#include "ili9488.h"
#endif

#ifdef USE_ST7789
display_t *display = (display_t *)&st7789v_display;
#endif

#ifdef USE_ILI9488
display_t *display = (display_t *)&ili9488_display;
#endif
