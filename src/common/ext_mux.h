#pragma once

#ifdef __cplusplus
extern "C" {
#endif //__cplusplus

extern void ext_mux_configure(int pin_select_a, int pin_select_b, int pin_channel_a, int pin_channel_b);

extern void ext_mux_select(int logic_input);

#ifdef __cplusplus
}
#endif //__cplusplus
