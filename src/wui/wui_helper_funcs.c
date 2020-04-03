#include "wui_helper_funcs.h"
#include "http_client.h"
#include "jsmn.h"
#include "wui.h"
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include "eeprom.h"
#include "ip4_addr.h"

//#define HTTP_DUBAI_HACK

#ifdef HTTP_DUBAI_HACK
    #include "version.h"
#endif

#define MAX_ACK_SIZE 16

static char buffer[MAX_REQ_BODY_SIZE] = "";

extern osMessageQId tcp_wui_mpool_id;
extern osSemaphoreId tcp_wui_semaphore_id;

static int json_cmp(const char *json, jsmntok_t *tok, const char *s) {
    if (tok->type == JSMN_STRING && (int)strlen(s) == tok->end - tok->start && strncmp(json + tok->start, s, tok->end - tok->start) == 0) {
        return 0;
    }
    return -1;
}

void send_request_to_wui(const char *request) {
    size_t req_len = strlen(request);
    osMessageQId queue = 0;
    const char *curr_ptr = request;
    uint16_t helper = 0;

    osSemaphoreWait(tcp_wui_semaphore_id, osWaitForever);
    if (0 != tcp_wui_queue_id) // queue valid
    {
        uint32_t q_space = osMessageAvailableSpace(tcp_wui_queue_id);

        wui_cmd_t *mptr;
        mptr = osPoolAlloc(tcp_wui_mpool_id);
        strlcpy(mptr->gcode_cmd, request, 100);
        osMessagePut(tcp_wui_queue_id, (uint32_t)mptr, osWaitForever); // Send Message
        osDelay(100);
    }
    osSemaphoreRelease(tcp_wui_semaphore_id);
}

void http_json_parser(char *json, uint32_t len) {
    int ret;
    jsmn_parser parser;
    jsmntok_t t[128]; // Just a raw value, we do not expect more that 128 tokens
    char request[MAX_REQ_MARLIN_SIZE];

    jsmn_init(&parser);
    ret = jsmn_parse(&parser, json, len, t, sizeof(t) / sizeof(jsmntok_t));

    if (ret < 1 || t[0].type != JSMN_OBJECT) {
        // Fail to parse JSON or top element is not an object
        return;
    }

    for (int i = 0; i < ret; i++) {
#ifdef HTTP_DUBAI_HACK
        if (json_cmp(json, &t[i], project_firmware_name) == 0) {
#else
        if (json_cmp(json, &t[i], "command") == 0) {
#endif //HTTP_DUBAI_HACK
            strlcpy(request, json + t[i + 1].start, (t[i + 1].end - t[i + 1].start + 1));
            i++;
            send_request_to_wui(request);
        } else if (json_cmp(json, &t[i], "connect_ip") == 0) {
            strlcpy(request, json + t[i + 1].start, t[i + 1].end - t[i + 1].start + 1);
            ip4_addr_t tmp_addr;
            if (ip4addr_aton(request, &tmp_addr)) {
                char connect_request[MAX_REQ_MARLIN_SIZE];
                snprintf(connect_request, MAX_REQ_MARLIN_SIZE, "!cip %lu", tmp_addr.addr);
                send_request_to_wui(connect_request);
            }
            i++;
        } else if (json_cmp(json, &t[i], "connect_key") == 0) {
            strlcpy(request, json + t[i + 1].start, t[i + 1].end - t[i + 1].start + 1);
            char connect_request[MAX_REQ_MARLIN_SIZE];
            snprintf(connect_request, MAX_REQ_MARLIN_SIZE, "!ck %s", request);
            send_request_to_wui(connect_request);
            i++;
        } else if (json_cmp(json, &t[i], "connect_name") == 0) {
            strlcpy(request, json + t[i + 1].start, t[i + 1].end - t[i + 1].start + 1);
            char connect_request[MAX_REQ_MARLIN_SIZE];
            snprintf(connect_request, MAX_REQ_MARLIN_SIZE, "!cn %s", request);
            send_request_to_wui(connect_request);
            i++;
        }
    }
}

uint32_t httpc_json_parser(char *json, uint32_t len, char *cmd_str) {
    uint32_t ret_code = 1; // success is 0
    int ret;
    jsmn_parser parser;
    jsmntok_t t[128]; // Just a raw value, we do not expect more that 128 tokens
    char request[MAX_REQ_MARLIN_SIZE];

    jsmn_init(&parser);
    ret = jsmn_parse(&parser, json, len, t, sizeof(t) / sizeof(jsmntok_t));

    if (ret < 1 || t[0].type != JSMN_OBJECT) {
        // Fail to parse JSON or top element is not an object
        return 1;
    }

    for (int i = 0; i < ret; i++) {
        if (json_cmp(json, &t[i], "command") == 0) {
            strlcpy(request, json + t[i + 1].start, (t[i + 1].end - t[i + 1].start + 1));
            i++;
            send_request_to_wui(request);
            ret_code = 0;
        } else if (json_cmp(json, &t[i], "connect_ip") == 0) {
            strlcpy(request, json + t[i + 1].start, t[i + 1].end - t[i + 1].start + 1);
            ip4_addr_t tmp_addr;
            if (ip4addr_aton(request, &tmp_addr)) {
                char connect_request[MAX_REQ_MARLIN_SIZE];
                snprintf(connect_request, MAX_REQ_MARLIN_SIZE, "!cip %lu", tmp_addr.addr);
                send_request_to_wui(connect_request);
                ret_code = 0;
            }
            i++;
        } else if (json_cmp(json, &t[i], "connect_key") == 0) {
            strlcpy(request, json + t[i + 1].start, t[i + 1].end - t[i + 1].start + 1);
            char connect_request[MAX_REQ_MARLIN_SIZE];
            snprintf(connect_request, MAX_REQ_MARLIN_SIZE, "!ck %s", request);
            send_request_to_wui(connect_request);
            ret_code = 0;
            i++;
        } else if (json_cmp(json, &t[i], "connect_name") == 0) {
            strlcpy(request, json + t[i + 1].start, t[i + 1].end - t[i + 1].start + 1);
            char connect_request[MAX_REQ_MARLIN_SIZE];
            snprintf(connect_request, MAX_REQ_MARLIN_SIZE, "!cn %s", request);
            send_request_to_wui(connect_request);
            ret_code = 0;
            i++;
        }
    }
    return ret_code;
}

const char *char_streamer(const char *format, ...) {
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, MAX_REQ_BODY_SIZE, format, args);
    va_end(args);
    return (const char *)&buffer;
}
