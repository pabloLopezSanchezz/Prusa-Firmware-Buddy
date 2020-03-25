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

wui_err buddy_http_client_init(uint8_t id, connect_event_t * container);

void buddy_http_client_loop();

#ifdef __cplusplus
}
#endif // __cplusplus

#endif //HTTP_CLIENT_H
