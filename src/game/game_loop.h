#ifndef GAME_LOOP_H
#define GAME_LOOP_H

#include <stdint.h>
#include <stdbool.h>

#include "platform/platform.h"
#include "assets/bal_palette.h"
#include "assets/bal_tiles.h"
#include "assets/bal_map.h"
#include "assets/bal_sprites.h"
#include "camera.h"
#include "hud.h"
#include "entities.h"
#include "house.h"
#include "house_ui.h"
#include "menu.h"
#include "render/bal_cursor.h"

typedef enum {
    APP_STATE_MENU = 0,
    APP_STATE_PLAYING = 1
} app_state_t;

typedef struct game_state_s {
    app_state_t state;
    menu_state_t menu;

    bal_palette_t palette;
    bal_tileset_t tileset;
    bal_map_t map;
    bal_sprites_t sprites;
    bal_cursor_t cursor;

    camera_t camera;
    hud_state_t hud_state;
    hud_resources_t hud_res;
    entity_manager_t entity_mgr;
    house_manager_t house_mgr;
    house_ui_t house_ui;
    int escape_cooldown;

    baldie_t *selected_unit;
    baldie_t *held_unit; // Unit currently picked up in the Hand
    int mouse_x;
    int mouse_y;
    bool mouse_down;
    int grab_x;
    int grab_y;
    int held_anim_timer;

    bool is_area_selecting;
    int area_start_x;
    int area_start_y;

    uint32_t current_level;
    bool running;
} game_state_t;

bool game_init(game_state_t *game, uint32_t level_num, int vp_w, int vp_h, bool start_in_menu);
bool game_load_level(game_state_t *game, uint32_t level_num, int vp_w, int vp_h);
void game_tick(game_state_t *game, const platform_input_t *input, int vp_w, int vp_h);
void game_render(game_state_t *game, surface_t *dest);
void game_shutdown(game_state_t *game);
void game_drop_held_unit(game_state_t *game, float world_mx, float world_my);

#endif // GAME_LOOP_H

