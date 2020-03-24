// screen_test_hx711.c

#include "config.h"

#ifdef LOADCELL_HX711

    #include <stdio.h>
    #include <stdlib.h>
    #include "gui.h"
    #include "loadcell_hx711.h"
    #include "gpio.h"
    #include "eeprom.h"

    #pragma pack(push)
    #pragma pack(1)

typedef struct
{
    window_frame_t frame;
    window_text_t text_terminal;
    window_text_t text_scale;
    window_spin_t spin_scale;
    window_text_t text_thrs;
    window_spin_t spin_thrs;
    window_text_t text_hyst;
    window_spin_t spin_hyst;
    window_text_t text_out;
    window_text_t button_tare;
    window_text_t button_save;
    window_text_t button_return;
    char str_out[32];
    char str_term[32];
    #ifdef FILAMENT_SENSOR_HX711
    window_text_t text_terminal2;
    char str_term2[32];
    #endif
} screen_test_hx711_data_t;

    #pragma pack(pop)

    #define pd ((screen_test_hx711_data_t *)screen->pdata)

void screen_test_hx711_init(screen_t *screen) {
    int16_t id;

    uint16_t y;
    uint16_t x;
    uint16_t w;
    uint16_t h;

    //font_t* _font_big = resource_font(IDR_FNT_BIG);
    font_t *_font_term = resource_font(IDR_FNT_TERMINAL);

    int16_t id0 = window_create_ptr(WINDOW_CLS_FRAME, -1, rect_ui16(0, 0, 0, 0), &(pd->frame));

    y = 0;
    x = 10;
    w = 220;
    h = 22;

    id = window_create_ptr(WINDOW_CLS_TEXT, id0, rect_ui16(x, y, w, h), &(pd->text_terminal));
    pd->text_terminal.font = _font_term;

    y += 22;
    #ifdef FILAMENT_SENSOR_HX711
    id = window_create_ptr(WINDOW_CLS_TEXT, id0, rect_ui16(x, y, w, h), &(pd->text_terminal2));
    pd->text_terminal2.font = _font_term;
    #endif
    y += 22;

    w = 140;

    id = window_create_ptr(WINDOW_CLS_TEXT, id0, rect_ui16(x, y, w, h), &(pd->text_scale));
    pd->text_scale.font = _font_term;
    window_set_text(id, "scale []");

    y += 22;

    id = window_create_ptr(WINDOW_CLS_SPIN, id0, rect_ui16(x, y, w, h), &(pd->spin_scale));
    window_set_format(id, "%.1f");
    window_set_min_max_step(id, 2.0F, 30.0F, 0.10F);
    window_set_value(id, loadcell_scale * 1000);
    window_set_tag(id, 4);

    y += 22;

    id = window_create_ptr(WINDOW_CLS_TEXT, id0, rect_ui16(x, y, w, h), &(pd->text_thrs));
    pd->text_thrs.font = _font_term;
    window_set_text(id, "threshold [g]");

    y += 22;

    id = window_create_ptr(WINDOW_CLS_SPIN, id0, rect_ui16(x, y, w, h), &(pd->spin_thrs));
    window_set_format(id, "%.0f");
    window_set_min_max_step(id, -500.0F, -5.0F, 5.0F);
    window_set_value(id, loadcell_threshold);
    window_set_tag(id, 5);

    y += 22;

    id = window_create_ptr(WINDOW_CLS_TEXT, id0, rect_ui16(x, y, w, h), &(pd->text_hyst));
    pd->text_hyst.font = _font_term;
    window_set_text(id, "hysteresis [g]");

    y += 22;

    id = window_create_ptr(WINDOW_CLS_SPIN, id0, rect_ui16(x, y, w, h), &(pd->spin_hyst));
    window_set_format(id, "%.0f");
    window_set_min_max_step(id, 0.0F, 100.0F, 5.0F);
    window_set_value(id, loadcell_hysteresis);
    window_set_tag(id, 6);

    y += 22;

    id = window_create_ptr(WINDOW_CLS_TEXT, id0, rect_ui16(x, y, w, h), &(pd->text_out));
    pd->text_out.font = _font_term;

    y += 22;

    id = window_create_ptr(WINDOW_CLS_TEXT, id0, rect_ui16(x, y, w, h), &(pd->button_tare));
    window_set_text(id, "Tare");
    window_enable(id);
    window_set_tag(id, 1);

    y += 22;

    id = window_create_ptr(WINDOW_CLS_TEXT, id0, rect_ui16(x, y, w, h), &(pd->button_save));
    window_set_text(id, "Save");
    window_enable(id);
    window_set_tag(id, 2);

    y += 22;

    id = window_create_ptr(WINDOW_CLS_TEXT, id0, rect_ui16(x, y, w, h), &(pd->button_return));
    window_set_text(id, "Return");
    window_enable(id);
    window_set_tag(id, 3);

    /*
	calibration_factor = st25dv64k_user_read(CALIBRATION_FACTOR_PROBE_ADDRESS);
	PROBE_HVAL = st25dv64k_user_read(PROBE_HVAL_PROBE_ADDRESS);
	HYSTERESIS = st25dv64k_user_read(HYSTERESIS_PROBE_ADDRESS);
	MODE = st25dv64k_user_read(HX711_MODE_ADRESS);


	if(MODE == 1)
	{
		window_set_text(id_mode, (const char*)"Inverted");
		mode_num = -1;
	}else
	{
		window_set_text(id_mode, (const char*)"Normal");
		mode_num = 1;
	}
*/
}

void screen_test_hx711_done(screen_t *screen) {
    window_destroy(pd->frame.win.id);
}

void screen_test_hx711_draw(screen_t *screen) {
}

int screen_test_hx711_event(screen_t *screen, window_t *window, uint8_t event, void *param) {
    if (event == WINDOW_EVENT_CLICK) {
        switch ((int)param) {
        case 1: // tare
            loadcell_tare();
            break;
        case 2: // save
            /*			st25dv64k_user_write(CALIBRATION_FACTOR_PROBE_ADDRESS, calibration_factor);
			st25dv64k_user_write(PROBE_HVAL_PROBE_ADDRESS, PROBE_HVAL);
			st25dv64k_user_write(HYSTERESIS_PROBE_ADDRESS, HYSTERESIS);
			st25dv64k_user_write(HX711_MODE_ADRESS, MODE);
			window_set_text(pd->text_saved.win.id, (const char*)"Saved :)");*/
            break;
        case 3: // return
            screen_close();
            return 1;
        }
    } else if (event == WINDOW_EVENT_CHANGE) {
        switch ((int)param) {
        case 4: //scale
            loadcell_scale = window_get_value(pd->spin_scale.window.win.id) / 1000;
            eeprom_set_var(EEVAR_LOADCELL_SCALE, variant8_flt(loadcell_scale));
            break;
        case 5: //thrs
            loadcell_threshold = window_get_value(pd->spin_thrs.window.win.id);
            eeprom_set_var(EEVAR_LOADCELL_THRS, variant8_flt(loadcell_threshold));
            break;
        case 6: //hyst
            loadcell_hysteresis = window_get_value(pd->spin_hyst.window.win.id);
            eeprom_set_var(EEVAR_LOADCELL_HYST, variant8_flt(loadcell_hysteresis));
            break;
        }
    } else if (event == WINDOW_EVENT_LOOP) {
        sprintf(pd->str_out, "%ld", loadcell_value);
        window_set_text(pd->text_out.win.id, pd->str_out);
        sprintf(pd->str_term, "LC: %d, RAW: %.1f", loadcell_probe, (double)loadcell_load);
        window_set_text(pd->text_terminal.win.id, pd->str_term);
    #ifdef FILAMENT_SENSOR_HX711
        sprintf(pd->str_term2, "FS: %d, RAW: %ld", fsensor_probe, fsensor_value);
        window_set_text(pd->text_terminal2.win.id, pd->str_term2);
    #endif
        /*
		calibration_factor = window_get_value(id_spin_scale);
		PROBE_HVAL = window_get_value(id_spin_thrs);
		HYSTERESIS = window_get_value(id_spin_hyst);

		sprintf(out, "Result mes: %s g", _out_p);
		window_set_text(id_probe, (const char*)out);

		ret_p = strtol(_out_p, &ptr_p, 10);

		switch (probe_hit)
		{
			case 0:
				if ((ret_p * mode_num) >= PROBE_HVAL)
				{
					probe_hit = 1;
					led_val = 0;        //ON
					window_set_text(id_out, (const char*)"OUT: 1");
				}
				break;

			case 1:
				if ((ret_p * mode_num) < (PROBE_HVAL - HYSTERESIS))
				{
					probe_hit = 0;
					led_val = 1;         // OFF
					window_set_text(id_out, (const char*)"OUT: 0");
				}
				break;
		}*/
    }
    return 0;
}

screen_t screen_test_hx711 = {
    0,
    0,
    screen_test_hx711_init,
    screen_test_hx711_done,
    screen_test_hx711_draw,
    screen_test_hx711_event,
    sizeof(screen_test_hx711_data_t), //data_size
    0,                                //pdata
};

const screen_t *pscreen_test_hx711 = &screen_test_hx711;

#endif //LOADCELL_HX711
