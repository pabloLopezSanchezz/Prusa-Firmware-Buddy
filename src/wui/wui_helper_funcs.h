#ifndef WUI_HELPER_FUNCS_H
#define WUI_HELPER_FUNCS_H

#include "cmsis_os.h"

#define MAX_REQ_MARLIN_SIZE 100
#define MAX_REQ_BODY_SIZE   512

void http_json_parser(char *json, uint32_t len);
void http_lowlvl_gcode_parser(const char * request, uint32_t length);
const char *char_streamer(const char *format, ...);

#endif //WUI_HELPER_FUNCS_H
