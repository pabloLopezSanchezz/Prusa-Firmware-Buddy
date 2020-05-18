#include "wui_request_parser.h"
#include "wui_helper_funcs.h"
#include <string.h>
#include "jsmn.h"
#include "dbg.h"

#define HTTP_DUBAI_HACK 0

#if HTTP_DUBAI_HACK
    #include "version.h"
#endif

#define CMD_LIMIT 10 // number of commands accepted in low level command response

#define MAX_ACK_SIZE 16

static int json_cmp(const char *json, jsmntok_t *tok, const char *s) {
    if (tok->type == JSMN_STRING && (int)strlen(s) == tok->end - tok->start && strncmp(json + tok->start, s, tok->end - tok->start) == 0) {
        return 0;
    }
    return -1;
}

uint32_t httpd_json_parser(char *json, uint32_t len) {
    int ret;
    jsmn_parser parser;
    jsmntok_t t[128]; // Just a raw value, we do not expect more that 128 tokens

    jsmn_init(&parser);
    ret = jsmn_parse(&parser, json, len, t, sizeof(t) / sizeof(jsmntok_t));

    if (ret < 1 || t[0].type != JSMN_OBJECT) {
        // Fail to parse JSON or top element is not an object
        return 0;
    }

    for (int i = 1; i < ret; i++) {
        if (t[i].size >= MAX_REQ_MARLIN_SIZE) {
            // Request is too long
            return 0;
        }
    }

    for (int i = 0; i < ret; i++) {
        wui_cmd_t request;
#if HTTP_DUBAI_HACK
        if (json_cmp(json, &t[i], project_firmware_name) == 0) {
#else
        if (json_cmp(json, &t[i], "command") == 0) {
#endif //HTTP_DUBAI_HACK
            strlcpy(request.arg, json + t[i + 1].start, t[i + 1].size + 1);
            request.lvl = LOW_LVL_CMD;
            i++;
            _dbg("command received: %s", request.arg);
            send_request_to_wui(&request);
        }
    }
    return 1;
}

static HTTPC_COMMAND_STATUS parse_high_level_cmd(char *json, uint32_t len) {
    int ret;
    jsmn_parser parser;
    jsmntok_t t[128]; // Just a raw value, we do not expect more that 128 tokens
    char request[MAX_REQ_MARLIN_SIZE];

    jsmn_init(&parser);
    ret = jsmn_parse(&parser, json, len, t, sizeof(t) / sizeof(jsmntok_t));

    if (ret < 1 || t[0].type != JSMN_OBJECT) {
        // Fail to parse JSON or top element is not an object
        return CMD_REJT_CMD_STRUCT;
    }

    wui_cmd_t command;
    HTTPC_COMMAND_STATUS ret_status = CMD_ACCEPTED;

    for (int i = 0; i < ret; i++) {
        if (json_cmp(json, &t[i], "command") == 0) {
            strlcpy(request, json + t[i + 1].start, (t[i + 1].end - t[i + 1].start + 1));
            i++;

            if (strcmp(request, "SEND_INFO") == 0) {
                command.high_lvl_cmd = CMD_SEND_INFO;
                ret_status = CMD_INFO_REQ; // REFACTOR in the future
                // TODO: other high level commands
            } else {
                command.high_lvl_cmd = CMD_UNKNOWN;
            }
        }
    }

    if (command.high_lvl_cmd != CMD_UNKNOWN && ret_status != CMD_INFO_REQ) {
        if (send_request_to_wui(&command)) {
            return CMD_REJT_NO_SPACE;
        }
    }

    return ret_status;
}

static HTTPC_COMMAND_STATUS parse_low_level_cmd(const char *request, httpc_header_info *h_info_ptr) {

    char gcode_str[CMD_LIMIT][MAX_REQ_MARLIN_SIZE] = { 0 };

    if (h_info_ptr->content_lenght <= 2) {
        return CMD_REJT_CMD_STRUCT;
    }

    uint32_t cmd_count = 0;
    uint32_t index = 0;
    const char *line_start_addr = request;
    const char *line_end_addr;
    uint32_t i = 0;

    do {
        cmd_count++;
        if (CMD_LIMIT < cmd_count) {
            return CMD_REJT_GCODES_LIMI; // return if more than 10 codes in the response
        }

        while ((i < h_info_ptr->content_lenght) && (request[i] != '\0') && (request[i] != '\r') && (request[i + 1] != '\n')) {
            i++;
        }

        line_end_addr = request + i;
        uint32_t str_len = line_end_addr - line_start_addr;
        strlcpy(gcode_str[index++], line_start_addr, str_len + 1);
        line_start_addr = line_end_addr + 2;
        i = i + 2; // skip the end of line chars

    } while (i < h_info_ptr->content_lenght);

    wui_cmd_t cmd;
    cmd.lvl = LOW_LVL_CMD;
    for (uint32_t cnt = 0; cnt < cmd_count; cnt++) {
        strcpy(cmd.arg, gcode_str[cnt]);
        if (send_request_to_wui(&cmd)) {
            return CMD_REJT_NO_SPACE;
        }
    }

    return CMD_ACCEPTED;
}

HTTPC_COMMAND_STATUS parse_http_reply(char *reply, uint32_t reply_len, httpc_header_info *h_info_ptr) {
    HTTPC_COMMAND_STATUS cmd_status = CMD_STATUS_UNKNOWN;
    if (0 == h_info_ptr->command_id) {
        return CMD_REJT_CMD_ID;
    }
    if (TYPE_JSON == h_info_ptr->content_type) {
        cmd_status = parse_high_level_cmd(reply, reply_len);
    } else if (TYPE_GCODE == h_info_ptr->content_type) {
        cmd_status = parse_low_level_cmd(reply, h_info_ptr);
    } else {
        cmd_status = CMD_REJT_CONT_TYPE;
    }
    return cmd_status;
}
