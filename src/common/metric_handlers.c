#include <stdarg.h>
#include "metric_handlers.h"
#include "stm32f4xx_hal.h"

// TODO: encapsulate huart6 access in hwio.h (and get rid of externs!)
extern UART_HandleTypeDef huart6;

static void uart_send_line(const char *fmt, ...) {
    char line[64];
    va_list va;
    va_start(va, fmt);
    int len = vsnprintf(line, sizeof(line), fmt, va);
    va_end(va);
    HAL_UART_Transmit(&huart6, (uint8_t *)line, len, HAL_MAX_DELAY);
    HAL_UART_Transmit(&huart6, (uint8_t *)"\n", 1, HAL_MAX_DELAY);
}

static void uart_handler(metric_point_t *point) {
    static int last_reported_timestamp = 0;
    int timestamp_diff = point->timestamp - last_reported_timestamp;
    last_reported_timestamp = point->timestamp;

    if (point->error) {
        uart_send_line("[%i:%s:error:%s]", timestamp_diff, point->metric->name,
            point->error_msg);
        return;
    }

    switch (point->metric->type) {
    case METRIC_VALUE_FLOAT:
        uart_send_line("[%i:%s:f:%f]", timestamp_diff, point->metric->name,
            (double)point->value_float);
        break;
    case METRIC_VALUE_INTEGER:
        uart_send_line("[%i:%s:i:%i]", timestamp_diff, point->metric->name,
            point->value_int);
        break;
    case METRIC_VALUE_STRING:
        // TODO: escaping
        uart_send_line("[%i:%s:s:%s]", timestamp_diff, point->metric->name,
            point->value_str);
        break;
    case METRIC_VALUE_EVENT:
        uart_send_line("[%i:%s:e:]", timestamp_diff, point->metric->name);
        break;
    default:
        uart_send_line("[%i:%s:error:unknown value type]", timestamp_diff, point->metric->name);
        break;
    }
}

metric_handler_t metric_handler_uart = {
    .identifier = METRIC_HANDLER_UART_ID,
    .name = "UART",
    .on_metric_registered_fn = NULL,
    .handle_fn = uart_handler,
};
