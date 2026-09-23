#include "game_loop.h"
#include "render/map_renderer.h"
#include "assets/bal_sfx.h"
#include "assets/bal_midi.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

bool game_init(game_state_t *game, uint32_t level_num, int vp_w, int vp_h) {
    if (!game) return false;
    memset(game, 0, sizeof(game_state_t));
    game->current_level = level_num;
    game->running = true;

    // 1. Load Map
    if (!bal_map_load(level_num, &game->map)) {
        printf("[ERROR] Failed to load level %u\n", level_num);
        return false;
    }

    // 2. Load World Palette
    char pal_path[64];
    snprintf(pal_path, sizeof(pal_path), "BALS/LEV%uPAL.BAL", (level_num > 5 ? 1 : level_num));
    if (!bal_palette_load(pal_path, &game->palette)) {
        // Fallback to LEV1PAL
        if (!bal_palette_load("BALS/LEV1PAL.BAL", &game->palette)) {
            printf("[ERROR] Failed to load world palette\n");
            return false;
        }
    }

    // 3. Load Tileset
    char blk_path[64];
    snprintf(blk_path, sizeof(blk_path), "BALS/%s.BAL", game->map.block_file);
    if (!bal_tileset_load(blk_path, &game->tileset)) {
        if (!bal_tileset_load("BALS/LEV1BLK.BAL", &game->tileset)) {
            printf("[ERROR] Failed to load tileset\n");
            return false;
        }
    }

    // 4. Initialize Camera
    camera_init(&game->camera, 
                game->map.start_cam_x, 
                game->map.start_cam_y, 
                game->map.width, 
                game->map.height, 
                vp_w, 
                vp_h - 32);

    // 5. Initialize HUD
    hud_init(&game->hud_res);
    game->hud_state.selected_role = 0; // Worker
    game->hud_state.minimap_visible = true;

    // 6. Initialize Entities & Houses
    entity_manager_init(&game->entity_mgr);
    house_manager_init(&game->house_mgr);

    // Spawn player base house near start camera
    int base_tx = (game->map.start_cam_x + 160) / 32;
    int base_ty = (game->map.start_cam_y + 160) / 32;
    house_t *player_base = house_create(&game->house_mgr, TEAM_PLAYER, HOUSE_HUT, base_tx, base_ty);
    if (player_base) {
        player_base->rooms[0] = 2; // 2 breeding workers inside
    }

    // Spawn initial player Baldies
    for (int i = 0; i < 4; i++) {
        float sx = (float)(base_tx * 32 + (i * 24) - 20);
        float sy = (float)(base_ty * 32 + 50);
        baldie_role_t role = (baldie_role_t)(i % 4);
        entity_spawn(&game->entity_mgr, TEAM_PLAYER, role, sx, sy);
    }

    // Spawn enemy base & Hairies further away
    int enemy_tx = base_tx + 30;
    int enemy_ty = base_ty + 20;
    if (enemy_tx < (int)game->map.width && enemy_ty < (int)game->map.height) {
        house_create(&game->house_mgr, TEAM_ENEMY, HOUSE_HUT, enemy_tx, enemy_ty);
        for (int i = 0; i < 3; i++) {
            entity_spawn(&game->entity_mgr, TEAM_ENEMY, ROLE_WORKER, (float)(enemy_tx * 32 + i * 20), (float)(enemy_ty * 32 + 50));
        }
    }

    // 7. Audio & Music
    bal_sfx_init("baldies.exe");
    char midi_path[64];
    snprintf(midi_path, sizeof(midi_path), "BALS/LEV%uMIDI.BAL", (level_num > 5 ? 1 : level_num));
    bal_midi_play(midi_path, true);

    return true;
}

void game_tick(game_state_t *game, const platform_input_t *input, int vp_w, int vp_h) {
    if (!game || !input) return;

    // 1. Camera update
    camera_update(&game->camera, input, vp_w, vp_h - 32);

    // 2. Role selection shortcuts (Keys 1-4)
    if (input->key_1) game->hud_state.selected_role = 0;
    if (input->key_2) game->hud_state.selected_role = 1;
    if (input->key_3) game->hud_state.selected_role = 2;
    if (input->key_4) game->hud_state.selected_role = 3;

    // Toggle minimap
    if (input->key_m) {
        game->hud_state.minimap_visible = !game->hud_state.minimap_visible;
    }

    // 3. Mouse interactions
    if (input->mouse_left_clicked) {
        int hud_y = vp_h - 32;
        if (input->mouse_y >= hud_y) {
            // Clicked inside HUD: check role boxes
            if (input->mouse_x >= 35 && input->mouse_x <= 75) game->hud_state.selected_role = 0;
            else if (input->mouse_x >= 135 && input->mouse_x <= 175) game->hud_state.selected_role = 1;
            else if (input->mouse_x >= 235 && input->mouse_x <= 275) game->hud_state.selected_role = 2;
            else if (input->mouse_x >= 335 && input->mouse_x <= 375) game->hud_state.selected_role = 3;
            bal_sfx_play(5); // UI click
        } else {
            // Clicked on game world: select nearest Baldie or assign order
            float world_mx = (float)(game->camera.x + input->mouse_x);
            float world_my = (float)(game->camera.y + input->mouse_y);

            // Find closest player unit
            baldie_t *closest = NULL;
            float min_dist = 24.0f; // Click radius
            for (int i = 0; i < MAX_BALDIES; i++) {
                baldie_t *b = &game->entity_mgr.units[i];
                if (!b->active || b->team != TEAM_PLAYER) continue;
                float d = fabsf(b->x - world_mx) + fabsf(b->y - world_my);
                if (d < min_dist) {
                    min_dist = d;
                    closest = b;
                }
            }

            if (closest) {
                game->selected_unit = closest;
                // Switch role to current selected role
                closest->role = (baldie_role_t)game->hud_state.selected_role;
                bal_sfx_play(2); // Confirmation voice
            } else if (game->selected_unit) {
                // Move selected unit to clicked location
                game->selected_unit->target_x = world_mx;
                game->selected_unit->target_y = world_my;
                bal_sfx_play(3); // Order voice
            }
        }
    }

    // 4. Update Simulation
    entity_update_all(&game->entity_mgr);
    house_update_all(&game->house_mgr, &game->entity_mgr);

    // 5. Update HUD counts
    uint32_t red = 0, blue = 0, white = 0, green = 0;
    for (int i = 0; i < MAX_BALDIES; i++) {
        const baldie_t *b = &game->entity_mgr.units[i];
        if (!b->active || b->team != TEAM_PLAYER) continue;
        switch (b->role) {
            case ROLE_WORKER:    red++; break;
            case ROLE_BUILDER:   blue++; break;
            case ROLE_SCIENTIST: white++; break;
            case ROLE_SOLDIER:   green++; break;
        }
    }
    // Include units inside houses
    for (int i = 0; i < MAX_HOUSES; i++) {
        const house_t *h = &game->house_mgr.houses[i];
        if (!h->active || h->team != TEAM_PLAYER) continue;
        red   += h->rooms[0];
        blue  += h->rooms[1];
        white += h->rooms[2];
        green += h->rooms[3];
    }
    game->hud_state.workers_red = red;
    game->hud_state.builders_blue = blue;
    game->hud_state.scientists_white = white;
    game->hud_state.soldiers_green = green;
}

void game_render(game_state_t *game, surface_t *dest) {
    if (!game || !dest) return;

    int vp_w = (int)dest->width;
    int vp_h = (int)dest->height;

    // 1. Draw World Map Tiles
    map_renderer_draw(dest, &game->map, &game->tileset, &game->palette, game->camera.x, game->camera.y);

    // 2. Draw Houses
    house_render_all(&game->house_mgr, dest, game->camera.x, game->camera.y);

    // 3. Draw Baldies & Hairies
    entity_render_all(&game->entity_mgr, dest, game->camera.x, game->camera.y, &game->palette);

    // 4. Draw Selected Unit Indicator
    if (game->selected_unit && game->selected_unit->active) {
        int sx = (int)game->selected_unit->x - game->camera.x;
        int sy = (int)game->selected_unit->y - game->camera.y;
        surface_draw_rect(dest, sx - 2, sy - 4, 20, 28, 0xFFFFFF00); // Yellow selection bounding box
    }

    // 5. Draw Mini-Map (top-right corner)
    if (game->hud_state.minimap_visible) {
        int mm_x = vp_w - (int)game->map.width - 12;
        int mm_y = 12;
        map_renderer_draw_minimap(dest, &game->map, &game->tileset, &game->palette, mm_x, mm_y, game->camera.x, game->camera.y, vp_w, vp_h - 32);
    }

    // 6. Draw HUD Interface (anchored at bottom)
    hud_render(dest, &game->hud_res, &game->hud_state, &game->palette);
}

void game_shutdown(game_state_t *game) {
    if (!game) return;
    bal_midi_stop();
    bal_sfx_shutdown();
    hud_free(&game->hud_res);
    bal_tileset_free(&game->tileset);
    bal_map_free(&game->map);
}
