//st7789v.h
#ifndef _ILI9488_H
#define _ILI9488_H

#include "display.h"
#include "stm32f4xx_hal.h"

//public flags (config)
#define ILI9488_FLG_DMA 0x08 // DMA enabled
#define ILI9488_FLG_MISO 0x10 // MISO enabled
#define ILI9488_FLG_SAFE 0x20 // SAFE mode (no DMA and safe delay)

#define ILI9488_DEF_COLMOD 0x66 // interface pixel format (6-6-6, hi-color)
#define ILI9488_DEF_MADCTL 0xE0 // memory data access control (mirror XY)

#pragma pack(push)
#pragma pack(1)

typedef struct _ili9488_config_t {
    SPI_HandleTypeDef *phspi; // spi handle pointer
    uint8_t pinCS; // CS pin
    uint8_t pinRS; // RS pin
    uint8_t pinRST; // RST pin
    uint8_t flg; // flags (DMA, MISO)
    uint8_t colmod; // interface pixel format
    uint8_t madctl; // memory data access control

    uint8_t gamma;
    uint8_t brightness;
    uint8_t is_inverted;
    uint8_t control;
} ili9488_config_t;

#pragma pack(pop)

#ifdef __cplusplus
extern "C" {
#endif //__cplusplus

extern void ili9488_inversion_on(void);
extern void ili9488_inversion_off(void);
extern void ili9488_inversion_tgl(void);
extern uint8_t ili9488_inversion_get(void);

extern void ili9488_gamma_next(void);
extern void ili9488_gamma_prev(void);
extern void ili9488_gamma_set(uint8_t gamma);
extern uint8_t ili9488_gamma_get(void);

extern void ili9488_brightness_set(uint8_t brightness);
extern uint8_t ili9488_brightness_get(void);

extern void ili9488_brightness_enable(void);
extern void ili9488_brightness_disable(void);

extern color_t ili9488_get_pixel(point_ui16_t pt);
extern void ili9488_set_pixel_directColor(point_ui16_t pt, uint32_t directColor);
extern uint32_t ili9488_get_pixel_directColor(point_ui16_t pt);

extern const display_t ili9488_display;

extern ili9488_config_t ili9488_config;

extern void ili9488_enable_safe_mode(void);

extern void ili9488_spi_tx_complete(void);

#ifdef __cplusplus
}
#endif //__cplusplus

#endif // _ILI9488_H
