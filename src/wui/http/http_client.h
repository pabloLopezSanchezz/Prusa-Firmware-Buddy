// http_client.h
// author: Migi

#ifndef HTTP_CLIENT_H
#define HTTP_CLIENT_H

#include "lwip/tcp.h"
#include "wui_err.h"
#include "wui_helper_funcs.h"

#ifdef __cplusplus
extern "C" {
#endif

// HTTP client request type
typedef enum {
    REQ_TELEMETRY, // Telemetry request
    REQ_EVENT,
    REQ_STATE_CHANGE,
    REQ_ACK, // acknowledgement of command reception
} HTTPC_REQ_TYPE;

typedef enum {
    EVENT_ACCEPTED,
    EVENT_REJECTED,
    EVENT_FINISHED,
    EVENT_STATE_CHANGED,
    EVENT_INFO,
} CONNECT_EVENT_TYPE;

typedef enum {
    TYPE_INVALID,
    TYPE_JSON,
    TYPE_GCODE
} HTTPC_CONTENT_TYPE;

typedef enum {
    CMD_UNKNOWN,
    CMD_ACCEPTED,
    CMD_REJT_GEN,
    CMD_REJT_SIZE,        // The response data size is larger than supported
    CMD_REJT_CMD_STRUCT,  // error in the command structure
    CMD_REJT_CMD_ID,      // error with Command-Id
    CMD_REJT_CDNT_TYPE,   // error with Content-Type
    CMD_REJT_GCODES_LIMI, // number of gcodes in x-gcode request exceeded
} HTTPC_COMMAND_STATUS;

typedef struct {
    const char name[100];
    HTTPC_COMMAND_STATUS status_code;
} httpc_cmd_status_str_t;

typedef struct {
    const char name[100];
    CONNECT_EVENT_TYPE event_code;
} httpc_con_event_str_t;

typedef struct {
    uint32_t command_id;
    uint32_t content_lenght;
    HTTPC_CONTENT_TYPE content_type;
} httpc_header_info;

typedef struct {
    HTTPC_REQ_TYPE req_type;
    CONNECT_EVENT_TYPE connect_event_type;
    uint32_t cmd_id;
    HTTPC_COMMAND_STATUS cmd_status;
} httpc_req_t;

wui_err buddy_http_client_req(httpc_req_t *request);

void send_request_to_httpc(httpc_req_t request);

void buddy_httpc_handler();
void buddy_httpc_handler_init();

#ifdef __cplusplus
}
#endif // __cplusplus

#endif //HTTP_CLIENT_H
