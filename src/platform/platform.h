#ifndef PLATFORM_H
#define PLATFORM_H

#include <stdint.h>
#include <stdbool.h>
#include "render/surface.h"

typedef struct {
    int mouse_x;
    int mouse_y;
    bool mouse_left_down;
    bool mouse_right_down;
    bool mouse_left_clicked;
    bool mouse_right_clicked;
    bool mouse_left_released;

    // Movement & camera inputs
    bool key_left;
    bool key_right;
    bool key_up;
    bool key_down;

    // Actions & shortcuts
    bool key_escape;
    bool key_escape_pressed;
    bool key_space;
    bool key_0;
    bool key_h;
    bool key_1;
    bool key_2;
    bool key_3;
    bool key_4;
    bool key_m; // Toggle minimap
    bool key_toggle_fullscreen;

    bool quit_requested;
} platform_input_t;

bool platform_init(const char *title, int width, int height, bool fullscreen);
void platform_poll_events(platform_input_t *out_input);
void platform_present(const surface_t *surface);
uint64_t platform_get_time_ms(void);
void platform_sleep_ms(uint32_t ms);
void platform_toggle_fullscreen(void);
void platform_get_window_size(int *out_w, int *out_h);
void platform_shutdown(void);

#endif // PLATFORM_H
