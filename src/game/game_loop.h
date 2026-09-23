#ifndef GAME_LOOP_H
#define GAME_LOOP_H

#include <stdint.h>
#include <stdbool.h>

#include "platform/platform.h"
#include "assets/bal_palette.h"
#include "assets/bal_tiles.h"
#include "assets/bal_map.h"
#include "camera.h"
#include "hud.h"
#include "entities.h"
#include "house.h"

typedef struct {
    bal_palette_t palette;
    bal_tileset_t tileset;
    bal_map_t map;

    camera_t camera;
    hud_state_t hud_state;
    hud_resources_t hud_res;
    entity_manager_t entity_mgr;
    house_manager_t house_mgr;

    baldie_t *selected_unit;
    uint32_t current_level;
    bool running;
} game_state_t;

bool game_init(game_state_t *game, uint32_t level_num, int vp_w, int vp_h);
void game_tick(game_state_t *game, const platform_input_t *input, int vp_w, int vp_h);
void game_render(game_state_t *game, surface_t *dest);
void game_shutdown(game_state_t *game);

#endif // GAME_LOOP_H
