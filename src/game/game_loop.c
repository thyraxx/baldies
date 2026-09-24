#include "game_loop.h"
#include "render/map_renderer.h"
#include "assets/bal_sfx.h"
#include "assets/bal_midi.h"
#include "pathfind.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

bool game_init(game_state_t *game, uint32_t level_num, int vp_w, int vp_h, bool start_in_menu) {
    if (!game) return false;
    memset(game, 0, sizeof(game_state_t));
    game->running = true;

    // Initialize Menu
    menu_init(&game->menu);
    if (level_num >= 1 && level_num <= 129) {
        game->menu.selected_level = level_num;
    }

    // Initialize HUD resources once
    hud_init(&game->hud_res);

    // Initialize Cursor
    bal_cursor_init(&game->cursor);

    // Initialize SFX
    bal_sfx_init("baldies.exe");

    if (start_in_menu) {
        game->state = APP_STATE_MENU;
        return true;
    } else {
        return game_load_level(game, level_num, vp_w, vp_h);
    }
}

bool game_load_level(game_state_t *game, uint32_t level_num, int vp_w, int vp_h) {
    if (!game) return false;

    // Free previous level resources if any
    bal_tileset_free(&game->tileset);
    bal_map_free(&game->map);
    bal_sprites_free(&game->sprites);

    game->current_level = level_num;
    uint32_t theme = ((level_num - 1) / 25) % 5 + 1;

    // 1. Load Map
    if (!bal_map_load(level_num, &game->map)) {
        printf("[ERROR] Failed to load level %u\n", level_num);
        return false;
    }

    // 2. Load World Palette
    char pal_path[64];
    snprintf(pal_path, sizeof(pal_path), "BALS/LEV%uPAL.BAL", theme);
    if (!bal_palette_load(pal_path, &game->palette)) {
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

    // 4. Load Sprites
    bal_sprites_init(&game->sprites, theme);

    // 5. Initialize Camera
    camera_init(&game->camera, 
                game->map.start_cam_x, 
                game->map.start_cam_y, 
                game->map.width, 
                game->map.height, 
                vp_w, 
                vp_h - HUD_HEIGHT);

    // 6. Reset HUD State
    game->hud_state.selected_role = 0; // Worker
    game->hud_state.minimap_visible = true;

    // 7. Initialize Entities & Houses
    entity_manager_init(&game->entity_mgr);
    house_manager_init(&game->house_mgr);
    game->selected_unit = NULL;

    // Spawn player base house at authentic level position
    int p_base_x = (int)game->map.player_base_x;
    int p_base_y = (int)game->map.player_base_y;

    house_t *player_base = house_create(&game->house_mgr, TEAM_PLAYER, HOUSE_HUT, p_base_x, p_base_y);
    if (player_base) {
        player_base->rooms[0] = 2; // 2 breeding workers inside
    }

    // Spawn initial player Baldies on the lawn outside the cottage
    for (int i = 0; i < 4; i++) {
        float sx = (float)(p_base_x - 12 + (i * 18));
        float sy = (float)(p_base_y + 54);
        baldie_role_t role = (baldie_role_t)(i % 4);
        entity_spawn(&game->entity_mgr, TEAM_PLAYER, role, sx, sy);
    }

    // Spawn enemy base & Hairies at authentic enemy island position
    int e_base_x = (int)game->map.enemy_base_x;
    int e_base_y = (int)game->map.enemy_base_y;

    house_create(&game->house_mgr, TEAM_ENEMY, HOUSE_HUT, e_base_x, e_base_y);
    for (int i = 0; i < 3; i++) {
        entity_spawn(&game->entity_mgr, TEAM_ENEMY, ROLE_WORKER, (float)(e_base_x - 8 + i * 18), (float)(e_base_y + 42));
    }



    // 8. Music
    char midi_path[64];
    snprintf(midi_path, sizeof(midi_path), "BALS/LEV%uMIDI.BAL", theme);
    bal_midi_play(midi_path, true);

    game->state = APP_STATE_PLAYING;
    return true;
}

void game_drop_held_unit(game_state_t *game, float world_mx, float world_my) {
    if (!game || !game->held_unit) return;

    baldie_t *b = game->held_unit;

    // 1. Check if dropped onto an active house
    house_t *target_house = NULL;
    for (int h_idx = 0; h_idx < MAX_HOUSES; h_idx++) {
        house_t *h = &game->house_mgr.houses[h_idx];
        if (!h->active) continue;
        if (world_mx >= (float)h->world_x && world_mx < (float)(h->world_x + 48) &&
            world_my >= (float)h->world_y && world_my < (float)(h->world_y + 48)) {
            target_house = h;
            break;
        }
    }

    if (target_house) {
        // Drop inside house!
        target_house->rooms[b->role]++;
        b->state = STATE_INSIDE_HOUSE;
        b->active = false;
        game->held_unit = NULL;
        bal_sfx_play(2); // Confirmation voice
        return;
    }

    // 2. Check if dropped onto deep ocean water
    int tx = (int)(world_mx / 16.0f);
    int ty = (int)(world_my / 16.0f);
    if (tx >= 0 && tx < (int)game->map.width && ty >= 0 && ty < (int)game->map.height) {
        uint16_t tile = bal_map_get_tile(&game->map, (uint32_t)tx, (uint32_t)ty);
        if (tile >= 340 && tile <= 359) {
            // Drowns in ocean!
            b->active = false;
            b->state = STATE_IDLE;
            game->held_unit = NULL;
            bal_sfx_play(11); // Splash sound
            return;
        }
    }

    // 3. Dropped onto ground or obstacle
    int place_tx = tx;
    int place_ty = ty;
    if (!pathfind_is_tile_walkable(&game->map, &game->tileset, &game->house_mgr, tx, ty)) {
        pathfind_find_nearest_walkable_tile(&game->map, &game->tileset, &game->house_mgr, tx, ty, &place_tx, &place_ty);
    }

    b->x = place_tx * 16.0f + 4.0f;
    b->y = place_ty * 16.0f + 2.0f;
    b->target_x = b->x;
    b->target_y = b->y;
    b->waypoint_count = 0;
    b->waypoint_index = 0;
    b->state = STATE_IDLE;
    game->held_unit = NULL;
    bal_sfx_play(5); // Drop sound
}

void game_tick(game_state_t *game, const platform_input_t *input, int vp_w, int vp_h) {
    if (!game || !input) return;

    if (game->state == APP_STATE_MENU) {
        if (input->key_escape) {
            game->running = false;
            return;
        }
        uint32_t chosen_level = 1;
        menu_action_t act = menu_update(&game->menu, input, vp_w, vp_h, &chosen_level);
        if (act == MENU_ACTION_START_LEVEL) {
            game_load_level(game, chosen_level, vp_w, vp_h);
        } else if (act == MENU_ACTION_QUIT) {
            game->running = false;
        }
        return;
    }

    // Return to menu on Escape if in playing state
    if (input->key_escape) {
        bal_midi_stop();
        game->state = APP_STATE_MENU;
        return;
    }

    // Track mouse position and held unit
    game->mouse_x = input->mouse_x;
    game->mouse_y = input->mouse_y;

    if (game->held_unit) {
        game->held_anim_timer++;
        game->held_unit->x = (float)(game->camera.x + input->mouse_x - 8);
        game->held_unit->y = (float)(game->camera.y + input->mouse_y - 8);
    }

    // 1. Camera update
    camera_update(&game->camera, input, vp_w, vp_h - HUD_HEIGHT);

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
        int clicked_role = hud_handle_click(input->mouse_x, input->mouse_y, vp_w, vp_h);
        if (clicked_role >= 0) {
            game->hud_state.selected_role = (uint32_t)clicked_role;
            if (game->held_unit) {
                game->held_unit->role = (baldie_role_t)clicked_role;
            }
            bal_sfx_play(5); // UI click
        } else if (input->mouse_y < vp_h - HUD_HEIGHT) {
            float world_mx = (float)(game->camera.x + input->mouse_x);
            float world_my = (float)(game->camera.y + input->mouse_y);

            if (game->held_unit) {
                // Drop / place the held Baldie!
                game_drop_held_unit(game, world_mx, world_my);
            } else {
                // Hand is empty: check if clicking on a friendly Baldie to pick up
                baldie_t *closest = NULL;
                float min_dist = 22.0f; // Click radius
                for (int i = 0; i < MAX_BALDIES; i++) {
                    baldie_t *b = &game->entity_mgr.units[i];
                    if (!b->active || b->team != TEAM_PLAYER || b->state == STATE_INSIDE_HOUSE || b->state == STATE_CARRIED) continue;
                    float d = fabsf(b->x + 8.0f - world_mx) + fabsf(b->y + 12.0f - world_my);
                    if (d < min_dist) {
                        min_dist = d;
                        closest = b;
                    }
                }

                if (closest) {
                    // Pick up Baldie into Hand!
                    game->held_unit = closest;
                    closest->state = STATE_CARRIED;
                    closest->waypoint_count = 0;
                    closest->waypoint_index = 0;
                    game->grab_x = input->mouse_x;
                    game->grab_y = input->mouse_y;
                    game->selected_unit = closest;
                    bal_sfx_play(2); // Confirmation voice
                } else if (game->selected_unit && game->selected_unit->active && game->selected_unit->state != STATE_CARRIED) {
                    // Pathfinding move order to destination
                    house_t *target_house = NULL;
                    int target_house_id = -1;
                    for (int h_idx = 0; h_idx < MAX_HOUSES; h_idx++) {
                        house_t *h = &game->house_mgr.houses[h_idx];
                        if (!h->active || h->team != game->selected_unit->team) continue;
                        if (world_mx >= (float)h->world_x && world_mx < (float)(h->world_x + 48) &&
                            world_my >= (float)h->world_y && world_my < (float)(h->world_y + 48)) {
                            target_house = h;
                            target_house_id = h_idx;
                            break;
                        }
                    }

                    float dest_x = world_mx;
                    float dest_y = world_my;

                    if (target_house) {
                        dest_x = (float)(target_house->world_x + 24);
                        dest_y = (float)(target_house->world_y + 40);
                        game->selected_unit->house_id = target_house_id;
                    } else {
                        game->selected_unit->house_id = -1;
                    }

                    path_result_t path;
                    float feet_x = game->selected_unit->x + 8.0f;
                    float feet_y = game->selected_unit->y + 12.0f;
                    if (pathfind_find_path(&game->map, &game->tileset, &game->house_mgr,
                                           feet_x, feet_y, dest_x, dest_y, &path)) {
                        float pts_x[MAX_PATH_NODES];
                        float pts_y[MAX_PATH_NODES];
                        for (int p = 0; p < path.count; p++) {
                            pts_x[p] = path.points[p].x;
                            pts_y[p] = path.points[p].y;
                        }
                        entity_set_path(game->selected_unit, pts_x, pts_y, path.count);
                        bal_sfx_play(3); // Order voice
                    }
                }
            }
        }
    } else if (input->mouse_left_released && game->held_unit) {
        // Drag-and-drop release support
        int drag_dist = abs(input->mouse_x - game->grab_x) + abs(input->mouse_y - game->grab_y);
        if (drag_dist > 12) {
            float world_mx = (float)(game->camera.x + input->mouse_x);
            float world_my = (float)(game->camera.y + input->mouse_y);
            game_drop_held_unit(game, world_mx, world_my);
        }
    }

    // 4. Update Simulation
    entity_update_all(&game->entity_mgr, &game->map, &game->tileset, &game->house_mgr);

    house_update_all(&game->house_mgr, &game->entity_mgr);

    if (game->selected_unit && !game->selected_unit->active) {
        game->selected_unit = NULL;
    }

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

    if (game->state == APP_STATE_MENU) {
        menu_render(&game->menu, dest);
        return;
    }

    int vp_w = (int)dest->width;
    int vp_h = (int)dest->height;

    // 1. Draw World Map Tiles
    map_renderer_draw(dest, &game->map, &game->tileset, &game->palette, game->camera.x, game->camera.y);

    // 2. Draw Houses
    house_render_all(&game->house_mgr, dest, game->camera.x, game->camera.y);

    // 3. Draw Baldies & Hairies (using authentic sprites!)
    entity_render_all(&game->entity_mgr, dest, game->camera.x, game->camera.y, &game->palette, &game->sprites);

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
        map_renderer_draw_minimap(dest, &game->map, &game->tileset, &game->palette, mm_x, mm_y, game->camera.x, game->camera.y, vp_w, vp_h - HUD_HEIGHT);
    }

    // 6. Draw HUD Interface (anchored at bottom)
    hud_render(dest, &game->hud_res, &game->hud_state, game->map.name, &game->palette);

    // 7. Draw In-Game Hand Cursor & Carried Unit
    if (game->cursor.data) {
        int cur_frame = CURSOR_FRAME_OPEN_HAND; // Frame 4: Open Hand
        if (game->held_unit) {
            cur_frame = CURSOR_FRAME_GRAB_HAND; // Frame 5: Closed Grabbing Hand

            // Draw carried Baldie kicking its legs beneath the hand
            int kick_frame = (game->held_anim_timer / 4) % 4;
            bal_sprites_draw_baldie(dest, &game->sprites, false, (int)game->held_unit->role, kick_frame,
                                    game->mouse_x - 8, game->mouse_y + 4, &game->palette);
        }
        // Draw hand cursor on top
        bal_cursor_draw(dest, &game->cursor, cur_frame, game->mouse_x - 12, game->mouse_y - 8, &game->palette);
    }
}

void game_shutdown(game_state_t *game) {
    if (!game) return;
    bal_midi_stop();
    bal_sfx_shutdown();
    menu_free(&game->menu);
    bal_sprites_free(&game->sprites);
    hud_free(&game->hud_res);
    bal_cursor_free(&game->cursor);
    bal_tileset_free(&game->tileset);
    bal_map_free(&game->map);
}

