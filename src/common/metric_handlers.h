#pragma once
#include "metric.h"

#ifdef __cplusplus
extern "C" {
#endif //__cplusplus

enum {
    METRIC_HANDLER_UART_ID,
};

extern metric_handler_t metric_handler_uart;

#ifdef __cplusplus
}
#endif //__cplusplus
