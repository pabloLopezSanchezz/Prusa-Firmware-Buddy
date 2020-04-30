#pragma once
#include "printers.h"
#include "stm32f4xx_hal.h"

#ifdef __cplusplus
extern "C" {
#endif //__cplusplus

extern int HAL_GPIO_Initialized;
extern int HAL_ADC_Initialized;
extern int HAL_PWM_Initialized;

extern TIM_HandleTypeDef htim1;
extern TIM_HandleTypeDef htim2;
extern TIM_HandleTypeDef htim3;

extern void Error_Handler(void);

#if (!defined(PRINTER_PRUSA_MINI) || !defined(PRINTER_PRUSA_MK4)             \
    || !defined(PRINTER_PRUSA_XL) || !defined(PRINTER_PRUSA_IXL)             \
    || !defined(PRINTER_PRUSA_MANIPULATOR) || !defined(PRINTER_PRUSA_PICKER) \
    || !defined(PRINTER_PRUSA_EXTRACTOR))
    #error "Some printer type not defined."
#endif
#if (PRINTER_TYPE == PRINTER_PRUSA_MINI)
    #include "main_MINI.h"
#elif (PRINTER_TYPE == PRINTER_PRUSA_MK4)
    #include "main_MK4.h"
#elif (PRINTER_TYPE == PRINTER_PRUSA_XL)
    #include "main_XL.h"
#elif (PRINTER_TYPE == PRINTER_PRUSA_IXL)
    #include "main_IXL.h"
#elif (PRINTER_TYPE == PRINTER_PRUSA_MANIPULATOR)
    #include "main_MANIPULATOR.h"
#elif (PRINTER_TYPE == PRINTER_PRUSA_PICKER)
    #include "main_PICKER.h"
#elif (PRINTER_TYPE == PRINTER_PRUSA_EXTRACTOR)
    #include "main_EXTRACTOR.h"
#else
    #error "Unknown PRINTER_TYPE!"
#endif

#ifdef FAN1_TACH_Pin
extern volatile uint32_t Tacho_FAN0;
#endif
#ifdef FAN0_TACH_Pin
extern volatile uint32_t Tacho_FAN1;
#endif

#ifdef __cplusplus
}
#endif //__cplusplus
