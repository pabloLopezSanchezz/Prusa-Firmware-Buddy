// screen_test_hx711.c

#include "config.h"

#ifdef LOADCELL_HX711

    #include <stdio.h>
    #include <stdlib.h>
    #include "gui.h"
    #include "loadcell.h"
    #include "gpio.h"
    #include "eeprom.h"

typedef struct
{
    window_frame_t frame;
    window_text_t text_terminal;
    window_text_t text_scale;
    window_spin_t spin_scale;
    window_text_t text_thrs_static;
    window_spin_t spin_thrs_static;
    window_text_t text_thrs_continuous;
    window_spin_t spin_thrs_continuous;
    window_text_t text_hyst;
    window_spin_t spin_hyst;
    window_text_t text_out;
    window_text_t button_save;
    window_text_t button_return;
    char str_out[32];
    char str_term[32];
    #ifdef FILAMENT_SENSOR_HX711
    window_text_t text_terminal2;
    char str_term2[32];
    #endif
} screen_test_hx711_data_t;

enum class Action : int {
    Tare = 1,
    Save,
    Close,
    UpdateThresholdStatic,
    UpdateThresholdContinous,
    UpdateScale,
    UpdateHysteresis,
};

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
    window_set_value(id, loadcell.GetScale() * 1000);
    window_set_tag(id, (int)Action::UpdateScale);

    y += 22;

    id = window_create_ptr(WINDOW_CLS_TEXT, id0, rect_ui16(x, y, w, h), &(pd->text_thrs_static));
    pd->text_thrs_static.font = _font_term;
    window_set_text(id, "threshold (MBL) [g]");

    y += 22;

    id = window_create_ptr(WINDOW_CLS_SPIN, id0, rect_ui16(x, y, w, h), &(pd->spin_thrs_static));
    window_set_format(id, "%.0f");
    window_set_min_max_step(id, -500.0F, -5.0F, 5.0F);
    window_set_value(id, loadcell.GetThreshold(Loadcell::TareMode::Static));
    window_set_tag(id, (int)Action::UpdateThresholdStatic);

    y += 22;

    id = window_create_ptr(WINDOW_CLS_TEXT, id0, rect_ui16(x, y, w, h), &(pd->text_thrs_continuous));
    pd->text_thrs_continuous.font = _font_term;
    window_set_text(id, "threshold (home) [g]");

    y += 22;

    id = window_create_ptr(WINDOW_CLS_SPIN, id0, rect_ui16(x, y, w, h), &(pd->spin_thrs_continuous));
    window_set_format(id, "%.0f");
    window_set_min_max_step(id, -500.0F, -5.0F, 5.0F);
    window_set_value(id, loadcell.GetThreshold(Loadcell::TareMode::Continuous));
    window_set_tag(id, (int)Action::UpdateThresholdContinous);

    y += 22;

    id = window_create_ptr(WINDOW_CLS_TEXT, id0, rect_ui16(x, y, w, h), &(pd->text_hyst));
    pd->text_hyst.font = _font_term;
    window_set_text(id, "hysteresis [g]");

    y += 22;

    id = window_create_ptr(WINDOW_CLS_SPIN, id0, rect_ui16(x, y, w, h), &(pd->spin_hyst));
    window_set_format(id, "%.0f");
    window_set_min_max_step(id, 0.0F, 100.0F, 5.0F);
    window_set_value(id, loadcell.GetHysteresis());
    window_set_tag(id, (int)Action::UpdateHysteresis);

    y += 22;

    id = window_create_ptr(WINDOW_CLS_TEXT, id0, rect_ui16(x, y, w, h), &(pd->text_out));
    pd->text_out.font = _font_term;

    y += 22;

    id = window_create_ptr(WINDOW_CLS_TEXT, id0, rect_ui16(x, y, w, h), &(pd->button_save));
    window_set_text(id, "Save");
    window_enable(id);
    window_set_tag(id, (int)Action::Save);

    y += 22;

    id = window_create_ptr(WINDOW_CLS_TEXT, id0, rect_ui16(x, y, w, h), &(pd->button_return));
    window_set_text(id, "Return");
    window_enable(id);
    window_set_tag(id, (int)Action::Close);
}

void screen_test_hx711_done(screen_t *screen) {
    window_destroy(pd->frame.win.id);
}

void screen_test_hx711_draw(screen_t *screen) {
}

int screen_test_hx711_event(screen_t *screen, window_t *window, uint8_t event, void *param) {
    if (event == WINDOW_EVENT_CLICK) {
        switch (Action((int)param)) {
        case Action::Save: // save
            eeprom_set_var(EEVAR_LOADCELL_SCALE, variant8_flt(loadcell.GetScale()));
            eeprom_set_var(EEVAR_LOADCELL_THRS_STATIC, variant8_flt(loadcell.GetThreshold(Loadcell::TareMode::Static)));
            eeprom_set_var(EEVAR_LOADCELL_THRS_CONTINOUS, variant8_flt(loadcell.GetThreshold(Loadcell::TareMode::Continuous)));
            eeprom_set_var(EEVAR_LOADCELL_HYST, variant8_flt(loadcell.GetHysteresis()));
            break;
        case Action::Close: // return
            screen_close();
            return 1;
        default:
            break;
        }
    } else if (event == WINDOW_EVENT_CHANGE) {
        switch (Action((int)param)) {
        case Action::UpdateScale:
            loadcell.SetScale(window_get_value(pd->spin_scale.window.win.id) / 1000);
            break;
        case Action::UpdateThresholdStatic:
            loadcell.SetThreshold(window_get_value(pd->spin_thrs_static.window.win.id), Loadcell::TareMode::Static);
            break;
        case Action::UpdateThresholdContinous:
            loadcell.SetThreshold(window_get_value(pd->spin_thrs_continuous.window.win.id), Loadcell::TareMode::Continuous);
            break;
        case Action::UpdateHysteresis:
            loadcell.SetHysteresis(window_get_value(pd->spin_hyst.window.win.id));
            break;
        default:
            break;
        }
    } else if (event == WINDOW_EVENT_LOOP) {
        sprintf(pd->str_out, "%ld", loadcell.GetRawValue());
        window_set_text(pd->text_out.win.id, pd->str_out);
        sprintf(pd->str_term, "LC: %d, RAW: %.1f", loadcell.GetMinZEndstop(), (double)loadcell.GetLoad());
        window_set_text(pd->text_terminal.win.id, pd->str_term);
    #ifdef FILAMENT_SENSOR_HX711
        //sprintf(pd->str_term2, "FS: %d, RAW: %ld", fsensor_probe, fsensor_value);
        window_set_text(pd->text_terminal2.win.id, pd->str_term2);
    #endif
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
