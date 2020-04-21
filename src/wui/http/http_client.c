/*
 * http_client.c
 * \brief
 *
 *  Created on: Feb 5, 2020
 *      Author: joshy <joshymjose[at]gmail.com>
 */

#include "http_client.h"
#include <stdbool.h>
#include "wui_api.h"
#include "stm32f4xx_hal.h"
#include <string.h>
#include "eeprom.h"
#include "lwip/altcp.h"
#include "lwip.h"
#include "marlin_vars.h"
#include "dbg.h"
#include "wui_api.h"

#define CLIENT_CONNECT_DELAY      1000 // 1000 = 1 Sec.
#define CONNECT_SERVER_PORT       8000
#define IP4_ADDR_STR_SIZE         16
#define REQ_HEADER_MAX_SIZE       256
#define REQ_BODY_MAX_SIZE         512
#define HTTPC_CONTENT_LEN_INVALID 0xFFFFFFFF
#define HTTPC_POLL_INTERVAL       1   // tcp_poll call interval (1 = 0.5 s)
#define HTTPC_POLL_TIMEOUT        3   // number of tcp_poll calls before quiting the current connection request (total time = HTTPC_POLL_TIMEOUT*HTTPC_POLL_INTERVAL)
#define HTTPC_RESPONSE_BUFF_SZ    512 // buffer size for http response from server on client request
#define HTTPC_REQUEST_BUFF_SZ     (REQ_HEADER_MAX_SIZE + REQ_BODY_MAX_SIZE)
#define WUI_HTTPC_Q_SZ            32 // WUI HTTPC queue size
#define CONENT_TYPE_STR_MAX_LEN   30 // content type allowed string length

static char httpc_req_header[REQ_HEADER_MAX_SIZE];
static char httpc_req_body[REQ_BODY_MAX_SIZE];
static char httpc_req_buffer[HTTPC_REQUEST_BUFF_SZ + 1] = "";   // buffer to make the request for HTTP request
static char httpc_resp_buffer[HTTPC_RESPONSE_BUFF_SZ + 1] = ""; // buffer to work with the response of HTTP request
static bool httpc_req_active = false;
static uint32_t client_interval = 0;
static bool init_tick = false;
static httpc_header_info header_info;

osSemaphoreId wui_httpc_semaphore_id = 0;

osMessageQDef(wui_httpc_queue, WUI_HTTPC_Q_SZ, httpc_req_t); // Define message queue
osMessageQId wui_httpc_queue_id = 0;

osPoolDef(httpc_req_mpool, WUI_HTTPC_Q_SZ, httpc_req_t);
osPoolId httpc_req_mpool_id;

static const httpc_cmd_status_str_t cmd_status_str[] = {
    { "General", CMD_REJT_GEN },
    { "Packet size overflow", CMD_REJT_SIZE },                  // The response data size is larger than supported
    { "error in the command structure", CMD_REJT_CMD_STRUCT },  // error in the command structure
    { "error with Command-Id", CMD_REJT_CMD_ID },               // error with Command-Id
    { "error with Content-Type", CMD_REJT_CDNT_TYPE },          // error with Content-Type
    { "number of gcodes exceeds limit", CMD_REJT_GCODES_LIMI }, // number of gcodes in x-gcode request exceeded
};

static const httpc_con_event_str_t conn_event_str[] = {
    { "ACCEPTED", EVENT_ACCEPTED },
    { "REJECTED", EVENT_REJECTED },
    { "FINISHED", EVENT_FINISHED },
    { "STATE_CHANGED", EVENT_STATE_CHANGED },
    { "INFO", EVENT_INFO },
};

/**
 * @ingroup httpc
 * HTTP client result codes
 */
typedef enum ehttpc_result {
    /** File successfully received */
    HTTPC_RESULT_OK = 0,
    /** Unknown error */
    HTTPC_RESULT_ERR_UNKNOWN = 1,
    /** Connection to server failed */
    HTTPC_RESULT_ERR_CONNECT = 2,
    /** Failed to resolve server hostname */
    HTTPC_RESULT_ERR_HOSTNAME = 3,
    /** Connection unexpectedly closed by remote server */
    HTTPC_RESULT_ERR_CLOSED = 4,
    /** Connection timed out (server didn't respond in time) */
    HTTPC_RESULT_ERR_TIMEOUT = 5,
    /** Server responded with an error code */
    HTTPC_RESULT_ERR_SVR_RESP = 6,
    /** Local memory error */
    HTTPC_RESULT_ERR_MEM = 7,
    /** Local abort */
    HTTPC_RESULT_LOCAL_ABORT = 8,
    /** Content length mismatch */
    HTTPC_RESULT_ERR_CONTENT_LEN = 9
} httpc_result_t;

typedef struct _httpc_state httpc_state_t;

/**
 * @ingroup httpc
 * Prototype of a http client callback function
 *
 * @param arg argument specified when initiating the request
 * @param httpc_result result of the http transfer (see enum httpc_result_t)
 * @param rx_content_len number of bytes received (without headers)
 * @param srv_res this contains the http status code received (if any)
 * @param err an error returned by internal lwip functions, can help to specify
 *            the source of the error but must not necessarily be != ERR_OK
 */
typedef void (*httpc_result_fn)(void *arg, httpc_result_t httpc_result, u32_t rx_content_len, u32_t srv_res, err_t err);

/**
 * @ingroup httpc
 * Prototype of http client callback: called when the headers are received
 *
 * @param connection http client connection
 * @param arg argument specified when initiating the request
 * @param hdr header pbuf(s) (may contain data also)
 * @param hdr_len length of the heders in 'hdr'
 * @param content_len content length as received in the headers (-1 if not received)
 * @return if != ERR_OK is returned, the connection is aborted
 */
typedef err_t (*httpc_headers_done_fn)(httpc_state_t *connection, void *arg, struct pbuf *hdr, u16_t hdr_len, u32_t content_len);

typedef struct _httpc_connection {
    ip_addr_t proxy_addr;
    u16_t proxy_port;
    u8_t use_proxy;
    /* @todo: add username:pass? */

#if LWIP_ALTCP
    altcp_allocator_t *altcp_allocator;
#endif

    /* this callback is called when the transfer is finished (or aborted) */
    httpc_result_fn result_fn;
    /* this callback is called after receiving the http headers
     It can abort the connection by returning != ERR_OK */
    httpc_headers_done_fn headers_done_fn;
} httpc_connection_t;

typedef enum ehttpc_parse_state {
    HTTPC_PARSE_WAIT_FIRST_LINE = 0,
    HTTPC_PARSE_WAIT_HEADERS,
    HTTPC_PARSE_RX_DATA
} httpc_parse_state_t;

typedef struct _httpc_state {
    struct altcp_pcb *pcb;
    ip_addr_t remote_addr;
    u16_t remote_port;
    int timeout_ticks;
    struct pbuf *request;
    struct pbuf *rx_hdrs;
    u16_t rx_http_version;
    u16_t rx_status;
    altcp_recv_fn recv_fn;
    const httpc_connection_t *conn_settings;
    void *callback_arg;
    u32_t rx_content_len;
    u32_t hdr_content_len;
    httpc_parse_state_t parse_state;
#if HTTPC_DEBUG_REQUEST
    char *server_name;
    char *uri;
#endif
} httpc_state_t;

static void send_request_to_httpc(httpc_req_t reqest) {

    osSemaphoreWait(wui_httpc_semaphore_id, osWaitForever);
    if (0 != wui_httpc_queue_id) // queue valid
    {
        uint32_t q_space = osMessageAvailableSpace(wui_httpc_queue_id);
        if (0 < q_space) {

            httpc_req_t *mptr;
            mptr = osPoolAlloc(httpc_req_mpool_id);
            *mptr = reqest;
            osMessagePut(wui_httpc_queue_id, (uint32_t)mptr, osWaitForever); // Send Message
            osDelay(100);
        } else {
            _dbg("httpc memory pool full!");
        }
    }
    osSemaphoreRelease(wui_httpc_semaphore_id);
}

/** Free http client state and deallocate all resources within */
static err_t
httpc_free_state(httpc_state_t *req) {
    struct altcp_pcb *tpcb;

    if (req->request != NULL) {
        pbuf_free(req->request);
        req->request = NULL;
    }
    if (req->rx_hdrs != NULL) {
        pbuf_free(req->rx_hdrs);
        req->rx_hdrs = NULL;
    }

    tpcb = req->pcb;
    mem_free(req);
    req = NULL;

    if (tpcb != NULL) {
        err_t r;
        altcp_arg(tpcb, NULL);
        altcp_recv(tpcb, NULL);
        altcp_err(tpcb, NULL);
        altcp_poll(tpcb, NULL, 0);
        altcp_sent(tpcb, NULL);
        r = altcp_close(tpcb);
        if (r != ERR_OK) {
            altcp_abort(tpcb);
            return ERR_ABRT;
        }
    }
    return ERR_OK;
}

/** Close the connection: call finished callback and free the state */
static err_t
httpc_close(httpc_state_t *req, httpc_result_t result, u32_t server_response, err_t err) {
    httpc_req_active = false;
    _dbg("closed httpc connection");
    if (req != NULL) {
        if (req->conn_settings != NULL) {
            if (req->conn_settings->result_fn != NULL) {
                req->conn_settings->result_fn(req->callback_arg, result, req->rx_content_len, server_response, err);
            }
        }
        return httpc_free_state(req);
    }
    return ERR_OK;
}

/** Parse http header response line 1 */
static err_t
http_parse_response_status(struct pbuf *p, u16_t *http_version, u16_t *http_status, u16_t *http_status_str_offset) {
    u16_t end1 = pbuf_memfind(p, "\r\n", 2, 0);
    if (end1 != 0xFFFF) {
        /* get parts of first line */
        u16_t space1, space2;
        space1 = pbuf_memfind(p, " ", 1, 0);
        if (space1 != 0xFFFF) {
            if ((pbuf_memcmp(p, 0, "HTTP/", 5) == 0) && (pbuf_get_at(p, 6) == '.')) {
                char status_num[10];
                size_t status_num_len;
                /* parse http version */
                u16_t version = pbuf_get_at(p, 5) - '0';
                version <<= 8;
                version |= pbuf_get_at(p, 7) - '0';
                *http_version = version;

                /* parse http status number */
                space2 = pbuf_memfind(p, " ", 1, space1 + 1);
                if (space2 != 0xFFFF) {
                    *http_status_str_offset = space2 + 1;
                    status_num_len = space2 - space1 - 1;
                } else {
                    status_num_len = end1 - space1 - 1;
                }
                memset(status_num, 0, sizeof(status_num));
                if (pbuf_copy_partial(p, status_num, (u16_t)status_num_len, space1 + 1) == status_num_len) {
                    int status = atoi(status_num);
                    if ((status > 0) && (status <= 0xFFFF)) {
                        *http_status = (u16_t)status;
                        return ERR_OK;
                    }
                }
            }
        }
    }
    return ERR_VAL;
}

/** Wait for all headers to be received, return its length and content-length (if available) */
static err_t
http_wait_headers(struct pbuf *p, u32_t *content_length, u16_t *total_header_len) {
    header_info.valid_request = false;
    header_info.content_type = TYPE_INVALID;
    header_info.content_lenght = 0;
    header_info.command_id = 0;

    u16_t header_end_pos = pbuf_memfind(p, "\r\n\r\n", 4, 0); // checks end of header

    if (header_end_pos < (0xFFFF - 2)) {
        /* all headers received */
        /* check if we have a content length (@todo: case insensitive?) */
        u16_t content_len_hdr;
        uint32_t parse_str_len = 0;
        *content_length = HTTPC_CONTENT_LEN_INVALID;
        *total_header_len = header_end_pos + 4;

        // parse Content-type
        const char *str_content_type = "Content-Type: ";
        parse_str_len = strlen(str_content_type);
        uint32_t content_type_hdr;

        content_type_hdr = pbuf_memfind(p, str_content_type, parse_str_len, 0); // 0 = search from begining
        if (content_type_hdr != 0xFFFF) {
            u16_t content_type_line_end = pbuf_memfind(p, "\r\n", 2, content_type_hdr);
            if (content_type_line_end != 0xFFFF) {
                char content_type_str[CONENT_TYPE_STR_MAX_LEN] = {};
                u16_t content_type_len = (u16_t)(content_type_line_end - content_type_hdr - parse_str_len);
                if (CONENT_TYPE_STR_MAX_LEN < content_type_len) {
                    return ERR_VAL;
                }

                if (pbuf_copy_partial(p, content_type_str, content_type_len, content_type_hdr + parse_str_len) == content_type_len) {
                    char *type_json_str = "application/json";
                    char *type_xgcode_str = "text/x.gcode";
                    if (0 == strncmp(content_type_str, type_json_str, strlen(type_json_str))) {
                        header_info.content_type = TYPE_JSON;
                    } else if (0 == strncmp(content_type_str, type_xgcode_str, strlen(type_xgcode_str))) {
                        header_info.content_type = TYPE_GCODE;
                    } else {
                        return ERR_VAL;
                    }
                }
            }
        }

        // prase Content-length
        const char *str_content_len = "Content-Length: ";
        parse_str_len = strlen(str_content_len);

        content_len_hdr = pbuf_memfind(p, str_content_len, parse_str_len, 0);
        if (content_len_hdr != 0xFFFF) {
            u16_t content_len_line_end = pbuf_memfind(p, "\r\n", 2, content_len_hdr);
            if (content_len_line_end != 0xFFFF) {
                char content_len_num[16];
                u16_t content_len_num_len = (u16_t)(content_len_line_end - content_len_hdr - parse_str_len);
                memset(content_len_num, 0, sizeof(content_len_num));
                if (pbuf_copy_partial(p, content_len_num, content_len_num_len, content_len_hdr + parse_str_len) == content_len_num_len) {
                    int len = atoi(content_len_num);
                    if ((len >= 0) && ((u32_t)len < HTTPC_CONTENT_LEN_INVALID)) {
                        *content_length = (u32_t)len;
                        header_info.content_lenght = (u32_t)len;
                    }
                }
            }
        }

        // prase Command-ld
        const char *str_cmd_id = "Command-Id: ";
        parse_str_len = strlen(str_cmd_id);

        u16_t command_id_hdr = pbuf_memfind(p, str_cmd_id, parse_str_len, 0);
        if (command_id_hdr != 0xFFFF) {
            u16_t command_id_line_end = pbuf_memfind(p, "\r\n", 2, command_id_hdr);
            if (command_id_line_end != 0xFFFF) {
                char command_id_num[16];
                u16_t command_id_num_len = (u16_t)(command_id_line_end - command_id_hdr - parse_str_len);
                memset(command_id_num, 0, sizeof(command_id_num));
                if (pbuf_copy_partial(p, command_id_num, command_id_num_len, command_id_hdr + parse_str_len) == command_id_num_len) {
                    header_info.command_id = atoi(command_id_num);
                }
            }
        }

        return ERR_OK;
    }
    return ERR_VAL;
}

/** http client tcp recv callback */
static err_t
httpc_tcp_recv(void *arg, struct altcp_pcb *pcb, struct pbuf *p, err_t r) {
    httpc_state_t *req = (httpc_state_t *)arg;
    LWIP_UNUSED_ARG(r);

    if (p == NULL) {
        httpc_result_t result;
        if (req->parse_state != HTTPC_PARSE_RX_DATA) {
            /* did not get RX data yet */
            result = HTTPC_RESULT_ERR_CLOSED;
        } else if ((req->hdr_content_len != HTTPC_CONTENT_LEN_INVALID) && (req->hdr_content_len != req->rx_content_len)) {
            /* header has been received with content length but not all data received */
            result = HTTPC_RESULT_ERR_CONTENT_LEN;
        } else {
            /* receiving data and either all data received or no content length header */
            result = HTTPC_RESULT_OK;
        }
        return httpc_close(req, result, req->rx_status, ERR_OK);
    }
    if (req->parse_state != HTTPC_PARSE_RX_DATA) {
        if (req->rx_hdrs == NULL) {
            req->rx_hdrs = p;
        } else {
            pbuf_cat(req->rx_hdrs, p);
        }
        if (req->parse_state == HTTPC_PARSE_WAIT_FIRST_LINE) {
            u16_t status_str_off;
            err_t err = http_parse_response_status(req->rx_hdrs, &req->rx_http_version, &req->rx_status, &status_str_off);
            if (err == ERR_OK) {
                /* don't care status string */
                req->parse_state = HTTPC_PARSE_WAIT_HEADERS;
            }
        }
        if (req->parse_state == HTTPC_PARSE_WAIT_HEADERS) {
            u16_t total_header_len;
            err_t err = http_wait_headers(req->rx_hdrs, &req->hdr_content_len, &total_header_len);
            if (err == ERR_OK) {
                struct pbuf *q;
                /* full header received, send window update for header bytes and call into client callback */
                altcp_recved(pcb, total_header_len);
                if (req->conn_settings) {
                    if (req->conn_settings->headers_done_fn) {
                        err = req->conn_settings->headers_done_fn(req, req->callback_arg, req->rx_hdrs, total_header_len, req->hdr_content_len);
                        if (err != ERR_OK) {
                            return httpc_close(req, HTTPC_RESULT_LOCAL_ABORT, req->rx_status, err);
                        }
                    }
                }
                /* hide header bytes in pbuf */
                q = pbuf_free_header(req->rx_hdrs, total_header_len);
                p = q;
                req->rx_hdrs = NULL;
                /* go on with data */
                req->parse_state = HTTPC_PARSE_RX_DATA;
            } else {
                pbuf_free(p);
            }
        }
    }
    if ((p != NULL) && (req->parse_state == HTTPC_PARSE_RX_DATA)) {
        req->rx_content_len += p->tot_len;
        if (req->recv_fn != NULL) {
            /* directly return here: the connection migth already be aborted from the callback! */
            return req->recv_fn(arg, pcb, p, r);
        } else {
            altcp_recved(pcb, p->tot_len);
            pbuf_free(p);
        }
    }
    return ERR_OK;
}

/** http client tcp err callback */
static void
httpc_tcp_err(void *arg, err_t err) {
    httpc_state_t *req = (httpc_state_t *)arg;
    if (req != NULL) {
        /* pcb has already been deallocated */
        req->pcb = NULL;
        httpc_close(req, HTTPC_RESULT_ERR_CLOSED, 0, err);
    }
}

/** http client tcp poll callback */
static err_t
httpc_tcp_poll(void *arg, struct altcp_pcb *pcb) {
    /* implement timeout */
    httpc_state_t *req = (httpc_state_t *)arg;
    LWIP_UNUSED_ARG(pcb);
    if (req != NULL) {
        if (req->timeout_ticks) {
            req->timeout_ticks--;
        }
        if (!req->timeout_ticks) {
            return httpc_close(req, HTTPC_RESULT_ERR_TIMEOUT, 0, ERR_OK);
        }
    }
    return ERR_OK;
}

/** http client tcp sent callback */
static err_t
httpc_tcp_sent(void *arg, struct altcp_pcb *pcb, u16_t len) {
    /* nothing to do here for now */
    LWIP_UNUSED_ARG(arg);
    LWIP_UNUSED_ARG(pcb);
    LWIP_UNUSED_ARG(len);
    return ERR_OK;
}

/** http client tcp connected callback */
static err_t
httpc_tcp_connected(void *arg, struct altcp_pcb *pcb, err_t err) {
    err_t r;
    httpc_state_t *req = (httpc_state_t *)arg;
    LWIP_UNUSED_ARG(pcb);
    LWIP_UNUSED_ARG(err);

    /* send request; last char is zero termination */
    r = altcp_write(req->pcb, req->request->payload, req->request->len - 1, TCP_WRITE_FLAG_COPY);
    if (r != ERR_OK) {
        /* could not write the single small request -> fail, don't retry */
        return httpc_close(req, HTTPC_RESULT_ERR_MEM, 0, r);
    }
    /* everything written, we can free the request */
    pbuf_free(req->request);
    req->request = NULL;

    altcp_output(req->pcb);
    return ERR_OK;
}

/** Function for tcp receive callback functions. Called when data has
 * been received.
 *
 * @param arg Additional argument to pass to the callback function (@see tcp_arg())
 * @param tpcb The connection pcb which received data
 * @param p The received data (or NULL when the connection has been closed!)
 * @param err An error code if there has been an error receiving
 *            Only return ERR_ABRT if you have called tcp_abort from within the
 *            callback function!
 */
err_t data_received_fun(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err) {

    LWIP_UNUSED_ARG(tpcb);
    LWIP_UNUSED_ARG(err);
    uint32_t len_copied = 0;
    HTTPC_COMMAND_STATUS cmd_status = CMD_UNKNOWN;
    httpc_state_t *req = (httpc_state_t *)arg;
    httpc_result_t result;

    memset(httpc_resp_buffer, 0, HTTPC_RESPONSE_BUFF_SZ); // reset the memory

    if (NULL == p) {
        return ERR_ARG;
    }

    if ((HTTPC_RESPONSE_BUFF_SZ < p->tot_len) || (HTTPC_RESPONSE_BUFF_SZ < header_info.content_lenght)) {
        cmd_status = CMD_REJT_SIZE;
        pbuf_free(p);
        return ERR_OK;
    }

    while (len_copied < p->tot_len) {

        char *payload = p->payload;
        // check if empty
        if (payload[0] == 0) {
            pbuf_free(p);
            return ERR_ARG;
        }

        len_copied += pbuf_copy_partial(p, httpc_resp_buffer, p->tot_len, 0);
        if (len_copied != p->tot_len) {
            p = p->next;
        }

        if (NULL == p) {
            break;
        }
    }

    pbuf_free(p);

    if (len_copied < header_info.content_lenght) {
        return ERR_ARG;
    }

    httpc_resp_buffer[header_info.content_lenght] = 0; // end of line added
    cmd_status = parse_http_reply(httpc_resp_buffer, len_copied, &header_info);
    result = HTTPC_RESULT_OK;
    u16_t status = req->rx_status;
    httpc_close(req, result, req->rx_status, ERR_OK);
    if (status == 200) {
        // send acknowledgment  TODO: only if it is answer to /p/telemetry
        if (CMD_UNKNOWN != cmd_status) {
            httpc_req_t request;
            request.cmd_id = header_info.command_id;
            request.cmd_status = cmd_status;
            request.req_type = REQ_ACK;
            if (CMD_ACCEPTED != cmd_status) {
                request.connect_event_type = EVENT_REJECTED;
            } else {
                request.connect_event_type = EVENT_ACCEPTED;
            }

            send_request_to_httpc(request);
        }
    } else if (status == 401 || status == 403) {
        // TODO: Error message to GUI: bad Printer-Token
    } else if (status == 404) {
        // TODO: Error message to GUI: bad connect address
    }

    return ERR_OK;
}

void get_ack_str(httpc_req_t *request, char *data, const uint32_t buf_len) {

    const char *event_name = conn_event_str[request->connect_event_type].name;

    if (EVENT_REJECTED == request->connect_event_type) {
        const char *reason = cmd_status_str[request->cmd_status].name;
        snprintf(data, buf_len, "{"
                                "\"event\":\"%s\","
                                "\"command_id\":%ld,"
                                "\"reason\":\"%s\""
                                "}",
            event_name, request->cmd_id, reason);
    } else if (EVENT_ACCEPTED == request->connect_event_type) {
        snprintf(data, buf_len, "{"
                                "\"event\":\"%s\","
                                "\"command_id\":%ld"
                                "}",
            event_name, request->cmd_id);
    }
}

static void create_http_header(char *http_header_str, uint32_t content_length, httpc_req_t *request) {
    _dbg("creating request header");
    char printer_token[CONNECT_TOKEN_SIZE + 1]; // extra space of end of line
    variant8_t printer_token_ptr = eeprom_get_var(EEVAR_CONNECT_TOKEN);
    strlcpy(printer_token, printer_token_ptr.pch, CONNECT_TOKEN_SIZE + 1);
    variant8_done(&printer_token_ptr);
#define STR_SIZE_MAX 50
    char uri[STR_SIZE_MAX] = { 0 };
    char content_type[STR_SIZE_MAX] = { 0 };

    switch (request->req_type) {
    case REQ_TELEMETRY:
        strlcpy(uri, "/p/telemetry", STR_SIZE_MAX);
        strlcpy(content_type, "application/json", STR_SIZE_MAX);
        break;
    case REQ_ACK:
        strlcpy(uri, "/p/events", STR_SIZE_MAX);
        strlcpy(content_type, "application/json", STR_SIZE_MAX);
        break;
    default:
        break;
    }
    snprintf(http_header_str, REQ_HEADER_MAX_SIZE - 1, "POST %s HTTP/1.0\r\nPrinter-Token: %s\r\nContent-Length: %lu\r\nContent-Type: %s\r\n\r\n", uri, printer_token, content_length, content_type);
}

static uint32_t get_reqest_body(char *http_body_str, httpc_req_t *request) {
    _dbg("creating request body");
    uint32_t content_length = 0;
    switch (request->req_type) {
    case REQ_TELEMETRY:
        get_telemetry_for_connect(httpc_req_body, REQ_BODY_MAX_SIZE);
        content_length = strlcpy(http_body_str, httpc_req_body, REQ_BODY_MAX_SIZE);
        break;
    case REQ_ACK:
        get_ack_str(request, httpc_req_body, REQ_BODY_MAX_SIZE);
        content_length = strlcpy(http_body_str, httpc_req_body, REQ_BODY_MAX_SIZE);
        break;
    default:
        break;
    }
    return content_length;
}

static const char *create_http_request(httpc_req_t *request) {
    // reset data
    memset(httpc_req_header, 0, REQ_HEADER_MAX_SIZE); // reset the memory
    memset(httpc_req_body, 0, REQ_BODY_MAX_SIZE);     // reset the memory

    uint32_t content_length = get_reqest_body(httpc_req_body, request);
    create_http_header(httpc_req_header, content_length, request);
    snprintf(httpc_req_buffer, HTTPC_REQUEST_BUFF_SZ, "%s%s", httpc_req_header, httpc_req_body);
    return (const char *)&httpc_req_buffer;
}

static wui_err buddy_http_client_req(httpc_req_t *request) {
    _dbg("creating client reqest");
    size_t alloc_len;
    mem_size_t mem_alloc_len;
    int req_len, req_len2;
    httpc_state_t *req;
    ip4_addr_t host_ip4;
    char host_ip4_str[IP4_ADDR_STR_SIZE];
    const char *header_plus_data;

    host_ip4.addr = eeprom_get_var(EEVAR_CONNECT_IP4).ui32;
    strlcpy(host_ip4_str, ip4addr_ntoa(&host_ip4), IP4_ADDR_STR_SIZE);

    header_plus_data = create_http_request(request);
    if (!header_plus_data) {
        return ERR_ARG;
    }

    req_len = strlen(header_plus_data);

    if ((req_len < 0) || (req_len > 0xFFFF)) {
        return ERR_VAL;
    }
    /* alloc state and request in one block */
    alloc_len = sizeof(httpc_state_t);
    mem_alloc_len = (mem_size_t)alloc_len;
    if ((mem_alloc_len < alloc_len) || (req_len + 1 > 0xFFFF)) {
        return ERR_VAL;
    }

    req = (httpc_state_t *)mem_malloc((mem_size_t)alloc_len);
    if (req == NULL) {
        return ERR_MEM;
    }
    memset(req, 0, sizeof(httpc_state_t));
    req->timeout_ticks = HTTPC_POLL_TIMEOUT;
    req->request = pbuf_alloc(PBUF_RAW, (u16_t)(req_len + 1), PBUF_RAM);
    if (req->request == NULL) {
        httpc_free_state(req);
        return ERR_MEM;
    }
    if (req->request->next != NULL) {
        /* need a pbuf in one piece */
        httpc_free_state(req);
        return ERR_MEM;
    }
    req->hdr_content_len = HTTPC_CONTENT_LEN_INVALID;
    req->pcb = altcp_new();
    if (req->pcb == NULL) {
        httpc_free_state(req);
        return ERR_MEM;
    }
    // setup callback functions
    altcp_arg(req->pcb, req);
    altcp_recv(req->pcb, httpc_tcp_recv);
    altcp_err(req->pcb, httpc_tcp_err);
    altcp_poll(req->pcb, httpc_tcp_poll, HTTPC_POLL_INTERVAL);
    altcp_sent(req->pcb, httpc_tcp_sent);
    req->recv_fn = data_received_fun; // callback when response data received

    /* set up request buffer */
    req_len2 = strlcpy((char *)req->request->payload, header_plus_data, req_len + 1);
    if (req_len2 != req_len) {
        httpc_free_state(req);
        return ERR_VAL;
    }

    tcp_connect(req->pcb, &host_ip4, CONNECT_SERVER_PORT, httpc_tcp_connected);
    return ERR_OK;
}

void buddy_httpc_handler() {

    if (eeprom_get_var(EEVAR_CONNECT_IP4).ui32 == 0) {
        return;
    }

    if (0 == netif_ip4_addr(&eth0)->addr) {
        return;
    }

    if (!httpc_req_active) {
        // check for any events to sent
        osEvent httpc_event = osMessageGet(wui_httpc_queue_id, 0);
        if (httpc_event.status == osEventMessage) {
            _dbg("sending event");
            httpc_req_t *rptr;
            rptr = httpc_event.value.p;
            if (NULL != httpc_event.value.p) {
                buddy_http_client_req(rptr);
                httpc_req_active = true;
            }
            osStatus status = osPoolFree(httpc_req_mpool_id, rptr); // free memory allocated for message
            if (osOK != status) {
                _dbg("wui_queue_pool free error: %d", status);
            }

        } else {
            if ((xTaskGetTickCount() - client_interval) > CLIENT_CONNECT_DELAY) {
                _dbg("sending telemtry");
                httpc_req_t req;
                req.req_type = REQ_TELEMETRY;
                buddy_http_client_req(&req);
                httpc_req_active = true;
                client_interval = xTaskGetTickCount();
            }
        }
    }
}

void buddy_httpc_handler_init() {
    // semaphore initalization
    osSemaphoreDef(wui_httpc_semaphore);
    wui_httpc_semaphore_id = osSemaphoreCreate(osSemaphore(wui_httpc_semaphore), 1);
    // memory pool initalization
    httpc_req_mpool_id = osPoolCreate(osPool(httpc_req_mpool));              // create memory pool
    wui_httpc_queue_id = osMessageCreate(osMessageQ(wui_httpc_queue), NULL); // create msg queue
    // for periodic telemetry request
    if (!init_tick) {
        client_interval = xTaskGetTickCount();
        init_tick = true;
    }
}
