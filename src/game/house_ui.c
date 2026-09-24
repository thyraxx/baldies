#include "house_ui.h"
#include "game_loop.h"
#include "assets/bal_sfx.h"
#include "pathfind.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void house_ui_init(house_ui_t *ui) {
    if (!ui) return;
    memset(ui, 0, sizeof(house_ui_t));
    ui->house_id = -1;
    ui->hovered_elem = -1;
    ui->hold_role = -1;
    ui->win_w = 420;
    ui->win_h = 270;
    ui->win_x = (640 - 420) / 2;
    ui->win_y = (432 - 270) / 2;
}

void house_ui_open(house_ui_t *ui, int house_id) {
    if (!ui) return;
    ui->is_open = true;
    ui->house_id = house_id;
    ui->hovered_elem = -1;
    ui->hold_role = -1;
    ui->hold_ticks = 0;
    ui->next_eject_delay = 0;
    ui->win_w = 420;
    ui->win_h = 270;
    ui->win_x = (640 - 420) / 2;
    ui->win_y = (432 - 270) / 2;
}

void house_ui_close(house_ui_t *ui) {
    if (!ui) return;
    ui->is_open = false;
    ui->house_id = -1;
    ui->hovered_elem = -1;
    ui->hold_role = -1;
    ui->hold_ticks = 0;
    ui->next_eject_delay = 0;
}

static void draw_beveled_box(surface_t *dest, int x, int y, int w, int h,
                             uint32_t bg_col, uint32_t top_col, uint32_t bot_col) {
    surface_fill_rect(dest, x, y, w, h, bg_col);
    // Top & left
    surface_fill_rect(dest, x, y, w, 1, top_col);
    surface_fill_rect(dest, x, y, 1, h, top_col);
    // Bottom & right shadow
    surface_fill_rect(dest, x, y + h - 1, w, 1, bot_col);
    surface_fill_rect(dest, x + w - 1, y, 1, h, bot_col);
}

bool house_ui_eject_unit(house_ui_t *ui, game_state_t *game, int role) {
    if (!ui || !game || ui->house_id < 0 || ui->house_id >= MAX_HOUSES) return false;
    house_t *h = &game->house_mgr.houses[ui->house_id];
    if (!h->active || role < 0 || role > 3) return false;
    if (h->rooms[role] <= 0) return false;

    // Door center is world_x + 24, world_y + 40
    // Lawn just outside front door is world_x + 20, world_y + 50
    float spawn_x = (float)(h->world_x + 20);
    float spawn_y = (float)(h->world_y + 50);

    // If slightly obstructed by another unit or terrain, find walkable offset
    if (!entity_is_position_walkable(&game->map, &game->tileset, &game->house_mgr, spawn_x, spawn_y)) {
        bool found = false;
        for (int r = 1; r <= 4 && !found; r++) {
            for (int dy = 0; dy <= r * 4 && !found; dy += 2) {
                for (int dx = -r * 4; dx <= r * 4 && !found; dx += 2) {
                    if (entity_is_position_walkable(&game->map, &game->tileset, &game->house_mgr, spawn_x + dx, spawn_y + dy)) {
                        spawn_x += dx;
                        spawn_y += dy;
                        found = true;
                    }
                }
            }
        }
    }

    baldie_t *b = entity_spawn(&game->entity_mgr, h->team, (baldie_role_t)role, spawn_x, spawn_y);
    if (!b) return false; // Could not spawn (e.g. max entities limit reached)

    h->rooms[role]--;
    b->facing = BALDIE_DIR_S;
    b->state = STATE_IDLE;
    b->anim_frame = 0;
    b->anim_timer = 0;
    b->target_x = spawn_x;
    b->target_y = spawn_y;
    b->waypoint_count = 0;
    b->waypoint_index = 0;

    bal_sfx_play(5); // Eject / door step sound
    return true;
}

bool house_ui_update(house_ui_t *ui, game_state_t *game, const platform_input_t *input, int vp_w, int vp_h) {
    if (!ui || !game || !input || !ui->is_open) return false;

    // Verify valid house
    if (ui->house_id < 0 || ui->house_id >= MAX_HOUSES || !game->house_mgr.houses[ui->house_id].active) {
        house_ui_close(ui);
        return false;
    }

    ui->anim_tick++;

    // Window dimensions (Centered within the playable viewport)
    int vp_play_h = vp_h - HUD_HEIGHT;
    ui->win_w = 420;
    ui->win_h = 270;
    ui->win_x = (vp_w - ui->win_w) / 2;
    ui->win_y = (vp_play_h - ui->win_h) / 2;
    if (ui->win_y < 10) ui->win_y = 10;

    int mx = input->mouse_x;
    int my = input->mouse_y;
    ui->hovered_elem = -1;

    // 1. Close Button [ X ] at top right: (win_x + win_w - 28, win_y + 6, 22x20)
    int close_x = ui->win_x + ui->win_w - 28;
    int close_y = ui->win_y + 6;
    if (mx >= close_x && mx <= close_x + 22 && my >= close_y && my <= close_y + 20) {
        ui->hovered_elem = 4;
    }

    // 2. Room Hitboxes (2 columns x 2 rows)
    int col_w = 194;
    int row_h = 98;
    int col0_x = ui->win_x + 10;
    int col1_x = ui->win_x + 216;
    int row0_y = ui->win_y + 36;
    int row1_y = ui->win_y + 140;

    // Room 0 (Workers - Red): Top-Left
    if (mx >= col0_x && mx <= col0_x + col_w && my >= row0_y && my <= row0_y + row_h) {
        ui->hovered_elem = 0;
    }
    // Room 1 (Builders - Blue): Top-Right
    else if (mx >= col1_x && mx <= col1_x + col_w && my >= row0_y && my <= row0_y + row_h) {
        ui->hovered_elem = 1;
    }
    // Room 2 (Scientists - White): Bottom-Left
    else if (mx >= col0_x && mx <= col0_x + col_w && my >= row1_y && my <= row1_y + row_h) {
        ui->hovered_elem = 2;
    }
    // Room 3 (Soldiers - Green): Bottom-Right
    else if (mx >= col1_x && mx <= col1_x + col_w && my >= row1_y && my <= row1_y + row_h) {
        ui->hovered_elem = 3;
    }

    // Escape closes House UI
    if (input->key_escape) {
        house_ui_close(ui);
        return true;
    }

    // 3. Handle Mouse Click
    if (input->mouse_left_clicked) {
        if (ui->hovered_elem == 4) { // Close button
            house_ui_close(ui);
            bal_sfx_play(5);
            return true;
        }

        if (ui->hovered_elem >= 0 && ui->hovered_elem < 4) {
            int role = ui->hovered_elem;
            house_t *h = &game->house_mgr.houses[ui->house_id];

            // If player has a Baldie picked up in hand, place unit into this room
            if (game->held_unit != NULL) {
                h->rooms[role]++;
                game->held_unit->active = false;
                game->held_unit->state = STATE_INSIDE_HOUSE;
                game->held_unit = NULL;
                bal_sfx_play(5);
                return true;
            }

            // Normal click on room: eject 1 unit and prepare continuous hold
            if (h->rooms[role] > 0) {
                house_ui_eject_unit(ui, game, role);
                ui->hold_role = role;
                ui->hold_ticks = 0;
                ui->next_eject_delay = 14; // ~460ms initial delay before continuous repeat begins
            }
            return true;
        }

        // If clicked outside the window boundaries, dismiss the house UI
        if (mx < ui->win_x || mx > ui->win_x + ui->win_w ||
            my < ui->win_y || my > ui->win_y + ui->win_h) {
            house_ui_close(ui);
            return true;
        }

        return true; // Click inside dialog consumed
    }

    // 4. Handle Holding Mouse Down (Accelerated Ejection)
    if (input->mouse_left_down) {
        if (ui->hold_role >= 0 && ui->hovered_elem == ui->hold_role) {
            house_t *h = &game->house_mgr.houses[ui->house_id];
            ui->hold_ticks++;
            ui->next_eject_delay--;
            if (ui->next_eject_delay <= 0) {
                if (h->rooms[ui->hold_role] > 0) {
                    house_ui_eject_unit(ui, game, ui->hold_role);

                    // Accelerating delay: starts at 7 ticks, gets faster down to 1 tick
                    int interval = 7;
                    if (ui->hold_ticks > 90) interval = 1;      // 30 units/sec (fastest)
                    else if (ui->hold_ticks > 50) interval = 2; // 15 units/sec
                    else if (ui->hold_ticks > 25) interval = 4; // 7.5 units/sec
                    ui->next_eject_delay = interval;
                } else {
                    ui->hold_role = -1;
                    ui->hold_ticks = 0;
                }
            }
        } else {
            ui->hold_role = -1;
            ui->hold_ticks = 0;
        }
    } else {
        ui->hold_role = -1;
        ui->hold_ticks = 0;
    }

    return true; // House UI intercepted input
}

void house_ui_render(const house_ui_t *ui, const game_state_t *game, surface_t *dest) {
    if (!ui || !game || !dest || !ui->is_open) return;
    if (ui->house_id < 0 || ui->house_id >= MAX_HOUSES) return;
    const house_t *h = &game->house_mgr.houses[ui->house_id];
    if (!h->active) return;

    int wx = ui->win_x;
    int wy = ui->win_y;
    int ww = ui->win_w;
    int wh = ui->win_h;

    // 1. Outer Dark Shadow & Multi-Layer Golden Beveled Frame
    surface_fill_rect(dest, wx - 3, wy - 3, ww + 6, wh + 6, 0xCC000000);
    surface_draw_rect(dest, wx - 2, wy - 2, ww + 4, wh + 4, 0xFF5C3A1E); // Bronze border
    surface_draw_rect(dest, wx - 1, wy - 1, ww + 2, wh + 2, 0xFFFFD700); // Radiant Gold border
    surface_draw_rect(dest, wx,     wy,     ww,     wh,     0xFF8B5A2B); // Inset Copper border

    // Background body: rich dark mahogany
    surface_fill_rect(dest, wx + 1, wy + 1, ww - 2, wh - 2, 0xEE160C08);

    // 2. Title Header Bar (wy + 2 to wy + 32)
    surface_fill_rect(dest, wx + 2, wy + 2, ww - 4, 30, 0xFF2A150C);
    surface_fill_rect(dest, wx + 2, wy + 31, ww - 4, 1, 0xFFDAA520);

    const char *tier_name = "HUT / COTTAGE";
    if (h->tier == HOUSE_BARRACKS) tier_name = "BARRACKS";
    else if (h->tier == HOUSE_CASTLE) tier_name = "CASTLE";

    char title_str[64];
    snprintf(title_str, sizeof(title_str), "%s INTERIOR", tier_name);
    surface_draw_text_shadow(dest, wx + 12, wy + 10, title_str, 0xFFFFD700, 0xFF000000, 1);

    int total_occupants = h->rooms[0] + h->rooms[1] + h->rooms[2] + h->rooms[3];
    char occ_str[48];
    snprintf(occ_str, sizeof(occ_str), "BALDIES INSIDE: %d", total_occupants);
    surface_draw_text_shadow(dest, wx + 195, wy + 10, occ_str, 0xFFE0D0B0, 0xFF000000, 1);

    // Close Button [ X ] (wy + 6, 22x20)
    int close_x = wx + ww - 28;
    int close_y = wy + 6;
    bool close_hover = (ui->hovered_elem == 4);
    uint32_t close_bg = close_hover ? 0xFF882020 : 0xFF402214;
    draw_beveled_box(dest, close_x, close_y, 22, 20, close_bg, 0xFFDAA520, 0xFF100804);
    surface_draw_text_shadow(dest, close_x + 6, close_y + 4, "X", close_hover ? 0xFFFFFFFF : 0xFFFFD700, 0xFF000000, 1);

    // 3. Four Rooms (2x2 grid)
    static const struct {
        const char *name;
        const char *subtitle;
        uint32_t bg_normal;
        uint32_t border_col;
        uint32_t hover_border;
        uint32_t swatch_col;
    } room_info[4] = {
        { "WORKERS (BREEDERS)",  "Breeds new Baldies",     0xCC281212, 0xFF882222, 0xFFFF4444, 0xFFFF3333 },
        { "BUILDERS",            "Builds & upgrades",      0xCC121C30, 0xFF2255AA, 0xFF4488FF, 0xFF3388FF },
        { "SCIENTISTS",          "Researches weapons",     0xCC202428, 0xFF667788, 0xFFCCDDEE, 0xFFEEEEEE },
        { "SOLDIERS",            "Guards & fights",        0xCC122416, 0xFF228833, 0xFF44DD55, 0xFF22DD22 }
    };

    int col_w = 194;
    int row_h = 98;
    int room_coords[4][2] = {
        { wx + 10,  wy + 36 }, // 0: Workers (Top-Left)
        { wx + 216, wy + 36 }, // 1: Builders (Top-Right)
        { wx + 10,  wy + 140 },// 2: Scientists (Bottom-Left)
        { wx + 216, wy + 140 } // 3: Soldiers (Bottom-Right)
    };

    for (int r = 0; r < 4; r++) {
        int rx = room_coords[r][0];
        int ry = room_coords[r][1];
        int count = h->rooms[r];
        bool is_hover = (ui->hovered_elem == r);
        bool is_held = (ui->hold_role == r && is_hover);

        // Room Background
        surface_fill_rect(dest, rx, ry, col_w, row_h, room_info[r].bg_normal);

        // Room Border
        uint32_t bcol = is_held ? 0xFFFFD700 : (is_hover ? room_info[r].hover_border : room_info[r].border_col);
        surface_draw_rect(dest, rx, ry, col_w, row_h, bcol);
        if (is_hover) {
            surface_draw_rect(dest, rx + 1, ry + 1, col_w - 2, row_h - 2, is_held ? 0xFFFFFFFF : room_info[r].hover_border);
        }

        // Color Swatch
        surface_fill_rect(dest, rx + 6, ry + 6, 10, 10, room_info[r].swatch_col);
        surface_draw_rect(dest, rx + 6, ry + 6, 10, 10, 0xFF000000);

        // Room Name
        surface_draw_text_shadow(dest, rx + 20, ry + 6, room_info[r].name, 0xFFFFD700, 0xFF000000, 1);

        // Subtitle / function description
        surface_draw_text(dest, rx + 20, ry + 17, room_info[r].subtitle, 0xFF999999, 1);

        // Visual Animated Baldie Figure
        int leg_frame = (count > 0) ? ((ui->anim_tick / 6) % 4) : 0;
        bal_sprites_draw_baldie(dest, &game->sprites, false, r, BALDIE_DIR_S, leg_frame, rx + 10, ry + 36, &game->palette);

        // Count Text (Clean scale 2 display)
        char cnt_str[32];
        snprintf(cnt_str, sizeof(cnt_str), "x %d", count);
        surface_draw_text_shadow(dest, rx + 32, ry + 36, cnt_str, count > 0 ? 0xFFFFFFFF : 0xFF777777, 0xFF000000, 2);

        // Draw additional mini Baldie figures standing inside room
        for (int b = 1; b < count && b <= 6; b++) {
            bal_sprites_draw_baldie(dest, &game->sprites, false, r, BALDIE_DIR_S, 0, rx + 90 + (b - 1) * 14, ry + 36, &game->palette);
        }

        // Room Action Button Area at bottom
        int btn_x = rx + 6;
        int btn_y = ry + 68;
        int btn_w = col_w - 12;
        int btn_h = 24;

        if (game->held_unit != NULL) {
            // Held unit hover: prompt dropping into this room
            draw_beveled_box(dest, btn_x, btn_y, btn_w, btn_h, 0xFF2A5020, 0xFF66FF44, 0xFF0A2008);
            surface_draw_text_shadow(dest, btn_x + 24, btn_y + 6, "[+] DROP UNIT HERE", 0xFFFFFFFF, 0xFF000000, 1);
        } else if (count > 0) {
            if (is_held) {
                // Held active leaving animation
                draw_beveled_box(dest, btn_x, btn_y, btn_w, btn_h, 0xFF5A2C10, 0xFF1A0A04, 0xFFDAA520);
                surface_draw_text_shadow(dest, btn_x + 36, btn_y + 6, ">> LEAVING... <<", 0xFFFFFFAA, 0xFF000000, 1);
            } else if (is_hover) {
                // Hovered button
                draw_beveled_box(dest, btn_x, btn_y, btn_w, btn_h, 0xFF452414, 0xFFFFD700, 0xFF200A04);
                surface_draw_text_shadow(dest, btn_x + 12, btn_y + 6, "[-] CLICK / HOLD TO LEAVE", 0xFFFFFFFF, 0xFF000000, 1);
            } else {
                // Normal button
                draw_beveled_box(dest, btn_x, btn_y, btn_w, btn_h, 0xFF2C160C, 0xFF8B5A2B, 0xFF140804);
                surface_draw_text_shadow(dest, btn_x + 36, btn_y + 6, "[-] LEAVE HOUSE", 0xFFE0D0B0, 0xFF000000, 1);
            }
        } else {
            // Empty room
            draw_beveled_box(dest, btn_x, btn_y, btn_w, btn_h, 0xFF140A06, 0xFF221108, 0xFF0A0402);
            surface_draw_text(dest, btn_x + 48, btn_y + 6, "(EMPTY ROOM)", 0xFF665544, 1);
        }
    }

    // 4. Footer Hint Bar
    surface_draw_text_shadow(dest, wx + 16, wy + 248,
                             "Click room to make Baldies leave  *  Hold mouse to speed up  *  ESC to close",
                             0xFFD4AF37, 0xFF000000, 1);
}
