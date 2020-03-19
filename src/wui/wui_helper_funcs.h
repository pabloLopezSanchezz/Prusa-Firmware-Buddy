#ifndef WUI_HELPER_FUNCS_H
#define WUI_HELPER_FUNCS_H

#include "cmsis_os.h"

#define MAX_REQ_MARLIN_SIZE 100
#define MAX_REQ_BODY_SIZE   512

#define MAX_STATE_LEN   10
#define MAX_REASON_LEN  50
typedef struct{
    char state[MAX_STATE_LEN];
    char reason[MAX_REASON_LEN];
    uint16_t command_id;
} connect_event_t;

void http_json_parser(char *json, uint32_t len);
void http_lowlvl_gcode_parser(const char * request, uint32_t length, uint16_t id);
const char *char_streamer(const char *format, ...);

#endif //WUI_HELPER_FUNCS_H
