/*
 * wui.h
 * \brief main interface functions for Web User Interface (WUI) thread
 *
 *  Created on: Dec 12, 2019
 *      Author: joshy
 */

#include "wui.h"
#include "wui_vars.h"
#include "marlin_client.h"
#include "lwip.h"
#include "ethernetif.h"
#include "http_client.h"
#include "eeprom.h"
#include <string.h>
#include "dbg.h"

#define MAX_WUI_REQUEST_LEN    100
#define MAX_MARLIN_REQUEST_LEN 100
#define WUI_FLG_PEND_REQ       0x0001
#define TCP_WUI_QUEUE_SIZE     64

osSemaphoreId tcp_wui_semaphore_id = 0;
osMessageQDef(tcp_wui_queue, TCP_WUI_QUEUE_SIZE, uint32_t);
osMessageQId tcp_wui_queue_id = 0;
osPoolDef(tcp_wui_mpool, TCP_WUI_QUEUE_SIZE, wui_cmd_t);
osPoolId tcp_wui_mpool_id;

osMutexDef(wui_thread_mutex);   // Mutex object for exchanging WUI thread TCP thread
osMutexId(wui_thread_mutex_id); // Mutex ID

static marlin_vars_t *wui_marlin_vars;
wui_vars_t wui_vars; // global vriable for data relevant to WUI

static void update_wui_vars(void) {
    osMutexWait(wui_thread_mutex_id, osWaitForever);
    wui_vars.pos[Z_AXIS_POS] = wui_marlin_vars->pos[Z_AXIS_POS];
    wui_vars.temp_nozzle = wui_marlin_vars->temp_nozzle;
    wui_vars.temp_bed = wui_marlin_vars->temp_bed;
    wui_vars.target_nozzle = wui_marlin_vars->target_nozzle;
    wui_vars.target_bed = wui_marlin_vars->target_bed;
    wui_vars.fan_speed = wui_marlin_vars->fan_speed;
    wui_vars.print_speed = wui_marlin_vars->print_speed;
    wui_vars.flow_factor = wui_marlin_vars->flow_factor;
    wui_vars.print_dur = wui_marlin_vars->print_duration;
    wui_vars.sd_precent_done = wui_marlin_vars->sd_percent_done;
    wui_vars.sd_printing = wui_marlin_vars->sd_printing;

    if (marlin_event(MARLIN_EVT_GFileChange)) {
        marlin_get_printing_gcode_name(wui_vars.gcode_name);
    }

    osMutexRelease(wui_thread_mutex_id);
}

static int process_wui_request(char *request) {

    if (strncmp(request, "!cip ", 5) == 0) {
        uint32_t ip;
        if (sscanf(request + 5, "%lu", &ip)) {
            eeprom_set_var(EEVAR_CONNECT_IP4, variant8_ui32(ip));
        }
    } else if (strncmp(request, "!ck ", 4) == 0) {
        variant8_t token = variant8_pchar(request + 4, 0, 0);
        eeprom_set_var(EEVAR_CONNECT_TOKEN, token);
        //variant8_done() is not called because variant_pchar with init flag 0 doesnt hold its memory
    } else if (strncmp(request, "!cn ", 4) == 0) {
        variant8_t hostname = variant8_pchar(request + 4, 0, 0);
        eeprom_set_var(EEVAR_LAN_HOSTNAME, hostname);
        //variant8_done() is not called because variant_pchar with init flag 0 doesnt hold its memory
    } else {
        _dbg("sending command: %s to marlin", request);
        marlin_gcode(request);
    }
    return 1;
}

static void wui_queue_cycle() {

    osEvent wui_event = osMessageGet(tcp_wui_queue_id, 0);

    if (wui_event.status == osEventMessage) {
        wui_cmd_t *rptr;
        rptr = wui_event.value.p;
        if (NULL != wui_event.value.p) {
            _dbg("command in wui queue");
            process_wui_request(rptr->gcode_cmd);
        }
        osStatus status = osPoolFree(tcp_wui_mpool_id, rptr); // free memory allocated for message
        if (osOK != status) {
            _dbg("wui_queue_pool free error: %d", status);
        }
    }
}

void StartWebServerTask(void const *argument) {
    // semaphore for filling tcp - wui message qeue
    osSemaphoreDef(tcp_wui_semaphore);
    tcp_wui_semaphore_id = osSemaphoreCreate(osSemaphore(tcp_wui_semaphore), 1);
    // message queue for commands from tcp thread to wui main loop
    tcp_wui_mpool_id = osPoolCreate(osPool(tcp_wui_mpool)); // create memory pool
    tcp_wui_queue_id = osMessageCreate(osMessageQ(tcp_wui_queue), NULL);

    // mutex for passing marlin variables to tcp thread
    wui_thread_mutex_id = osMutexCreate(osMutex(wui_thread_mutex));

    // marlin client initialization
    wui_marlin_vars = marlin_client_init(); // init the client
    // force update variables when starts
    marlin_client_set_event_notify(MARLIN_EVT_MSK_DEF - MARLIN_EVT_MSK_FSM);
    marlin_client_set_change_notify(MARLIN_VAR_MSK_DEF | MARLIN_VAR_MSK_WUI);
    // LwIP related initalizations
    MX_LWIP_Init();
    http_server_init();

#ifdef BUDDY_ENABLE_CONNECT
    buddy_httpc_handler_init();
#endif // BUDDY_ENABLE_CONNECT

    for (;;) {

        ethernetif_link(&eth0); // handles Ethernet link plug/un-plug events
        wui_queue_cycle();      // checks for commands to WUI

        if (wui_marlin_vars) {
            marlin_client_loop();
            update_wui_vars();
        }

#ifdef BUDDY_ENABLE_CONNECT
        buddy_httpc_handler();
#endif // BUDDY_ENABLE_CONNECT

        osDelay(100);
    }
}
