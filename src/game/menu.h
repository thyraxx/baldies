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
    uint8_t *title_pixels;   // 640x480 raw bytes from BALDTITL.BAL
    bal_palette_t palette;   // from BTPAL.BAL
    bal_palette_t game_pal;  // from LEV1PAL.BAL
    uint32_t selected_level; // 1 to 129
    int hovered_button;      // 0=none, 1=start, 2=prev, 3=next, 4=quit, 5=prev10, 6=next10
    bool mouse_down;

    // Cached map preview for currently selected level
    uint32_t preview_level;
    surface_t *preview_surf;
    char preview_name[32];
    char preview_theme[32];
    uint16_t preview_w;
    uint16_t preview_h;
    int p_base_x, p_base_y;  // player base tile coordinates
    int e_base_x, e_base_y;  // enemy base tile coordinates
    uint32_t anim_tick;
} menu_state_t;

bool menu_init(menu_state_t *menu);
void menu_free(menu_state_t *menu);
void menu_load_preview(menu_state_t *menu, uint32_t level_num);

menu_action_t menu_update(menu_state_t *menu, const platform_input_t *input, int screen_w, int screen_h, uint32_t *out_level);
void menu_render(menu_state_t *menu, surface_t *dest);

#endif // MENU_H
