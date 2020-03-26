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

typedef enum {
    REQ_TELEMETRY,
    REQ_EVENT,
} HTTP_CLIENT_REQ_TYPE;

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
    CMD_FINISHED
} HTTPC_COMMAND_STATUS;

typedef struct {
    uint32_t command_id;
    uint32_t content_lenght;
    HTTPC_CONTENT_TYPE content_type;
} httpc_header_info;

wui_err buddy_http_client_req(HTTP_CLIENT_REQ_TYPE reqest_type);

void buddy_http_client_loop();

#ifdef __cplusplus
}
#endif // __cplusplus

#endif //HTTP_CLIENT_H
