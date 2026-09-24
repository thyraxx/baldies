#ifndef HUD_H
#define HUD_H

#include <stdint.h>
#include <stdbool.h>
#include "render/surface.h"
#include "assets/bal_palette.h"

typedef struct {
    uint32_t workers_red;
    uint32_t builders_blue;
    uint32_t scientists_white;
    uint32_t soldiers_green;
    uint32_t selected_role; // 0=Red, 1=Blue, 2=White, 3=Green
    bool minimap_visible;
} hud_state_t;

typedef struct {
    uint8_t *hpan_data; // 640x32 raw frames from HPAN640.BAL
    uint32_t hpan_size;
} hud_resources_t;

bool hud_init(hud_resources_t *hud_res);
void hud_free(hud_resources_t *hud_res);

#define HUD_HEIGHT 48

void hud_render(surface_t *dest,
                const hud_resources_t *hud_res,
                const hud_state_t *hud_state,
                const char *level_name,
                const bal_palette_t *palette);

int hud_handle_click(int mouse_x, int mouse_y, int screen_w, int screen_h);

#endif // HUD_H
