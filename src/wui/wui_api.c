/*
 * wui_api.c
 * \brief
 *
 *  Created on: Jan 24, 2020
 *      Author: joshy <joshymjose[at]gmail.com>
 */

#include "wui_api.h"

#include "wui.h"
#include "filament.h"
#include "wui_vars.h"
#include "progress_data_wrapper.h"

#include "stdarg.h"

#define BDY_WUI_API_BUFFER_SIZE 512
#define CMD_LIMIT               10 // number of commands accepted in low level command response

// for data exchange between wui thread and HTTP thread
static wui_vars_t wui_vars_copy;

void get_telemetry_for_connect(char *data, const uint32_t buf_len) {

    osStatus status = osMutexWait(wui_thread_mutex_id, osWaitForever);
    if (status == osOK) {
        wui_vars_copy = wui_vars;
    }
    osMutexRelease(wui_thread_mutex_id);

    float z_pos_mm = wui_vars_copy.pos[Z_AXIS_POS];
    const char *filament_material = filaments[get_filament()].name;

    if (!wui_vars_copy.sd_printing) {
        snprintf(data, buf_len, "{"
                                "\"temp_nozzle\":%.2f,"
                                "\"temp_bed\":%.2f,"
                                "\"target_nozzle\":%.2f,"
                                "\"target_bed\":%.2f,"
                                "\"p_fan\":%d,"
                                "\"material\":\"%s\","
                                "\"pos_z_mm\":%.2f,"
                                "\"printing_speed\":%d,"
                                "\"flow_factor\":%d"
                                "}",
            wui_vars_copy.temp_nozzle, wui_vars_copy.temp_bed,
            wui_vars_copy.target_nozzle, wui_vars_copy.target_bed,
            wui_vars_copy.fan_speed, filament_material,
            z_pos_mm, wui_vars_copy.print_speed, wui_vars_copy.flow_factor);

         return;
    }

    uint8_t percent_done;
    uint32_t estimated_time;
    char time_2_end[9], print_time[13];
    if (is_percentage_valid(wui_vars_copy.print_dur)) {
        estimated_time = progress_time2end(wui_vars_copy.print_speed);
        percent_done = progress_get_percentage();
        progress_format_time2end(time_2_end, wui_vars_copy.print_speed);
    } else {
        estimated_time = 0;
        strlcpy(time_2_end, "N/A", 4);
        percent_done = wui_vars_copy.sd_precent_done;
    }

    print_dur_to_string(print_time, sizeof(print_time), wui_vars_copy.print_dur);

    snprintf(data, buf_len, "{"
                            "\"temp_nozzle\":%.2f,"
                            "\"temp_bed\":%.2f,"
                            "\"target_nozzle\":%.2f,"
                            "\"target_bed\":%.2f,"
                            "\"p_fan\":%d,"
                            "\"material\":\"%s\","
                            "\"pos_z_mm\":%.2f,"
                            "\"printing_speed\":%d,"
                            "\"flow_factor\":%d,"
                            "\"progress\":%d,"
                            "\"print_dur\":\"%s\","     // OctoPrint API ?
                            "\"time_est\":\"%s\","
                            "\"printing_time\":%ld,"     // Connect
                            "\"estimated_time\":%ld,"
                            "\"project_name\":\"%s\","
                            "\"state\":\"PRINTING\""
                            "}",
        wui_vars_copy.temp_nozzle, wui_vars_copy.temp_bed,
        wui_vars_copy.target_nozzle, wui_vars_copy.target_bed,
        wui_vars_copy.fan_speed, filament_material,
        z_pos_mm, wui_vars_copy.print_speed, wui_vars_copy.flow_factor,
        percent_done, print_time, time_2_end,
        wui_vars_copy.print_dur, estimated_time,
        wui_vars_copy.gcode_name);
}


void get_telemetry_for_local(char *data, const uint32_t buf_len) {

    osStatus status = osMutexWait(wui_thread_mutex_id, osWaitForever);
    if (status == osOK) {
        wui_vars_copy = wui_vars;
    }
    osMutexRelease(wui_thread_mutex_id);

    int32_t actual_nozzle = (int32_t)(wui_vars_copy.temp_nozzle);
    int32_t actual_heatbed = (int32_t)(wui_vars_copy.temp_bed);
    double z_pos_mm = (double)wui_vars_copy.pos[Z_AXIS_POS];
    uint16_t print_speed = (uint16_t)(wui_vars_copy.print_speed);
    uint16_t flow_factor = (uint16_t)(wui_vars_copy.flow_factor);
    const char *filament_material = filaments[get_filament()].name;

    if (!wui_vars_copy.sd_printing) {
        snprintf(data, buf_len, "{"
                                "\"temp_nozzle\":%ld,"
                                "\"temp_bed\":%ld,"
                                "\"material\":\"%s\","
                                "\"pos_z_mm\":%.2f,"
                                "\"printing_speed\":%d,"
                                "\"flow_factor\":%d"
                                "}",
            actual_nozzle, actual_heatbed, filament_material,
            z_pos_mm, print_speed, flow_factor);
            return;
    }

    uint8_t percent_done;
    char time_2_end[9], print_time[13];
    if (is_percentage_valid(wui_vars_copy.print_dur)) {
        percent_done = progress_get_percentage();
        progress_format_time2end(time_2_end, wui_vars_copy.print_speed);
    } else {
        strlcpy(time_2_end, "N/A", 4);
        percent_done = wui_vars_copy.sd_precent_done;
    }

    print_dur_to_string(print_time, sizeof(print_time), wui_vars_copy.print_dur);

    snprintf(data, buf_len, "{"
                            "\"temp_nozzle\":%ld,"
                            "\"temp_bed\":%ld,"
                            "\"material\":\"%s\","
                            "\"pos_z_mm\":%.2f,"
                            "\"printing_speed\":%d,"
                            "\"flow_factor\":%d,"
                            "\"progress\":%d,"
                            "\"print_dur\":\"%s\","
                            "\"time_est\":\"%s\","
                            "\"project_name\":\"%s\""
                            "}",
        actual_nozzle, actual_heatbed, filament_material,
        z_pos_mm, print_speed, flow_factor,
        percent_done, print_time, time_2_end, wui_vars_copy.gcode_name);
}


static HTTPC_COMMAND_STATUS parse_high_level_cmd(char *json, uint32_t len) {
    char cmd_str[200];
    uint32_t ret_code = httpc_json_parser(json, len, cmd_str);
    if (ret_code) {
        return CMD_REJT_CMD_STRUCT;
    } else {
        return CMD_ACCEPTED;
    }
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

    for (uint32_t cnt = 0; cnt < cmd_count; cnt++) {
        send_request_to_wui(gcode_str[cnt]);
    }

    return CMD_ACCEPTED;
}

HTTPC_COMMAND_STATUS parse_http_reply(char *reply, uint32_t reply_len, httpc_header_info *h_info_ptr) {
    HTTPC_COMMAND_STATUS cmd_status = CMD_UNKNOWN;
    if (0 == h_info_ptr->command_id) {
        return CMD_REJT_CMD_ID;
    }
    if (TYPE_JSON == h_info_ptr->content_type) {
        cmd_status = parse_high_level_cmd(reply, reply_len);
    } else if (TYPE_GCODE == h_info_ptr->content_type) {
        cmd_status = parse_low_level_cmd(reply, h_info_ptr);
    } else {
        cmd_status = CMD_REJT_CDNT_TYPE;
    }
    return cmd_status;
}
