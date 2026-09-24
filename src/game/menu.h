#ifndef MENU_H
#define MENU_H

#include <stdint.h>
#include <stdbool.h>
#include "render/surface.h"
#include "assets/bal_palette.h"
#include "platform/platform.h"

typedef enum {
    MENU_ACTION_NONE = 0,
    MENU_ACTION_START_LEVEL,
    MENU_ACTION_QUIT
} menu_action_t;

typedef struct {
    uint8_t *title_pixels; // 640x480 raw bytes from BALDTITL.BAL
    bal_palette_t palette; // from BTPAL.BAL
    bool in_level_select;
    uint32_t selected_level; // 1 to 129
} menu_state_t;

bool menu_init(menu_state_t *menu);
void menu_free(menu_state_t *menu);

menu_action_t menu_update(menu_state_t *menu, const platform_input_t *input, int screen_w, int screen_h, uint32_t *out_level);
void menu_render(menu_state_t *menu, surface_t *dest);

#endif // MENU_H
