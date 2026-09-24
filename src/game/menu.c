#include "menu.h"
#include "assets/asset_path.h"
#include "assets/bal_map.h"
#include "assets/bal_tiles.h"
#include "assets/bal_sfx.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

bool menu_init(menu_state_t *menu) {
    if (!menu) return false;
    memset(menu, 0, sizeof(menu_state_t));
    menu->selected_level = 1;

    // 1. Load Title Palette (BALS/BTPAL.BAL)
    if (!bal_palette_load("BALS/BTPAL.BAL", &menu->palette)) {
        return false;
    }

    // 2. Load Game Palette (BALS/LEV1PAL.BAL) for preview and UI accents
    if (!bal_palette_load("BALS/LEV1PAL.BAL", &menu->game_pal)) {
        menu->game_pal = menu->palette;
    }

    // 3. Load Title Image (640x480)
    FILE *f = asset_open_file("BALS/BALDTITL.BAL", "rb");
    if (!f) return false;

    menu->title_pixels = (uint8_t*)malloc(640 * 480);
    if (!menu->title_pixels) {
        fclose(f);
        return false;
    }

    size_t read_bytes = fread(menu->title_pixels, 1, 640 * 480, f);
    fclose(f);
    if (read_bytes != (640 * 480)) {
        return false;
    }

    // 4. Load initial level preview
    menu_load_preview(menu, 1);

    return true;
}

void menu_free(menu_state_t *menu) {
    if (!menu) return;
    if (menu->title_pixels) {
        free(menu->title_pixels);
        menu->title_pixels = NULL;
    }
    if (menu->preview_surf) {
        surface_destroy(menu->preview_surf);
        menu->preview_surf = NULL;
    }
}

void menu_load_preview(menu_state_t *menu, uint32_t level_num) {
    if (!menu) return;
    if (level_num < 1) level_num = 1;
    if (level_num > 129) level_num = 129;

    if (menu->preview_surf && menu->preview_level == level_num) {
        return; // Already cached
    }

    if (menu->preview_surf) {
        surface_destroy(menu->preview_surf);
        menu->preview_surf = NULL;
    }

    bal_map_t map;
    if (!bal_map_load(level_num, &map)) {
        return;
    }

    uint32_t theme = ((level_num - 1) / 25) % 5 + 1;
    const char *theme_name = "GRASSLANDS";
    uint32_t water_col = 0xFF143878; // Blue sea

    switch (theme) {
        case 1:
            theme_name = "GRASSLANDS";
            water_col = 0xFF143878;
            break;
        case 2:
            theme_name = "ICE & SNOW";
            water_col = 0xFF2A4A70;
            break;
        case 3:
            theme_name = "DESERT DUNES";
            water_col = 0xFF1E3250;
            break;
        case 4:
            theme_name = "HELLFIRE";
            water_col = 0xFF581008;
            break;
        case 5:
            theme_name = "ALIEN SPACE";
            water_col = 0xFF080614;
            break;
    }

    // Load theme palette
    bal_palette_t theme_pal;
    char pal_path[64];
    snprintf(pal_path, sizeof(pal_path), "BALS/LEV%uPAL.BAL", theme);
    if (!bal_palette_load(pal_path, &theme_pal)) {
        bal_palette_load("BALS/LEV1PAL.BAL", &theme_pal);
    }

    // Load tileset for preview
    bal_tileset_t tileset;
    char blk_path[64];
    snprintf(blk_path, sizeof(blk_path), "BALS/%s.BAL", map.block_file);
    if (!bal_tileset_load(blk_path, &tileset)) {
        bal_tileset_load("BALS/LEV1BLK.BAL", &tileset);
    }

    menu->preview_level = level_num;
    strncpy(menu->preview_name, map.name, sizeof(menu->preview_name) - 1);
    menu->preview_name[sizeof(menu->preview_name) - 1] = '\0';
    strncpy(menu->preview_theme, theme_name, sizeof(menu->preview_theme) - 1);
    menu->preview_theme[sizeof(menu->preview_theme) - 1] = '\0';

    menu->preview_w = map.width;
    menu->preview_h = map.height;
    menu->p_base_x = (int)(map.player_base_x / 16.0f);
    menu->p_base_y = (int)(map.player_base_y / 16.0f);
    menu->e_base_x = (int)(map.enemy_base_x / 16.0f);
    menu->e_base_y = (int)(map.enemy_base_y / 16.0f);

    // Create 120x120 thumbnail
    menu->preview_surf = surface_create(120, 120);
    if (menu->preview_surf) {
        surface_clear(menu->preview_surf, water_col);

        int ox = (120 - (int)map.width) / 2;
        int oy = (120 - (int)map.height) / 2;

        for (uint32_t ty = 0; ty < map.height; ty++) {
            for (uint32_t tx = 0; tx < map.width; tx++) {
                uint16_t tile = bal_map_get_tile(&map, tx, ty);
                if (tile >= 340 && tile <= 359) {
                    continue; // Skip water to let water background show
                }
                const uint8_t *tdata = bal_tileset_get_tile(&tileset, tile);
                if (tdata) {
                    uint8_t c = tdata[8 * 16 + 8]; // Sample center of tile
                    uint32_t col = theme_pal.argb[c];
                    int px = ox + (int)tx;
                    int py = oy + (int)ty;
                    if (px >= 0 && px < 120 && py >= 0 && py < 120) {
                        menu->preview_surf->pixels[py * 120 + px] = col;
                    }
                }
            }
        }
    }

    bal_tileset_free(&tileset);
    bal_map_free(&map);
}

static void draw_beveled_box(surface_t *dest, int x, int y, int w, int h,
                             uint32_t bg_col, uint32_t top_col, uint32_t bot_col) {
    surface_fill_rect(dest, x, y, w, h, bg_col);
    // Top & left highlight
    surface_fill_rect(dest, x, y, w, 1, top_col);
    surface_fill_rect(dest, x, y, 1, h, top_col);
    // Bottom & right shadow
    surface_fill_rect(dest, x, y + h - 1, w, 1, bot_col);
    surface_fill_rect(dest, x + w - 1, y, 1, h, bot_col);
}

menu_action_t menu_update(menu_state_t *menu, const platform_input_t *input, int screen_w, int screen_h, uint32_t *out_level) {
    (void)screen_w; (void)screen_h;
    if (!menu || !input) return MENU_ACTION_NONE;

    menu->anim_tick++;
    menu->mouse_down = input->mouse_left_down;

    // Button Geometry:
    // 1: START MISSION   (215, 352, 310, 46)
    // 2: PREV LEVEL <    (262, 304,  42, 34)
    // 3: NEXT LEVEL >    (434, 304,  42, 34)
    // 4: QUIT GAME       (215, 408, 310, 36)
    // 5: PREV 10 [ -10 ] (215, 304,  42, 34)
    // 6: NEXT 10 [ +10 ] (483, 304,  42, 34)
    int mx = input->mouse_x;
    int my = input->mouse_y;
    int hover = 0;

    if (mx >= 215 && mx <= 525 && my >= 352 && my <= 398) hover = 1;
    else if (mx >= 262 && mx <= 304 && my >= 304 && my <= 338) hover = 2;
    else if (mx >= 434 && mx <= 476 && my >= 304 && my <= 338) hover = 3;
    else if (mx >= 215 && mx <= 525 && my >= 408 && my <= 444) hover = 4;
    else if (mx >= 215 && mx <= 257 && my >= 304 && my <= 338) hover = 5;
    else if (mx >= 483 && mx <= 525 && my >= 304 && my <= 338) hover = 6;

    menu->hovered_button = hover;

    // Handle Keyboard Shortcuts
    if (input->key_left) {
        if (menu->selected_level > 1) {
            menu->selected_level--;
            bal_sfx_play(5);
            menu_load_preview(menu, menu->selected_level);
        }
    }
    if (input->key_right) {
        if (menu->selected_level < 129) {
            menu->selected_level++;
            bal_sfx_play(5);
            menu_load_preview(menu, menu->selected_level);
        }
    }
    if (input->key_down) {
        if (menu->selected_level > 10) menu->selected_level -= 10;
        else menu->selected_level = 1;
        bal_sfx_play(5);
        menu_load_preview(menu, menu->selected_level);
    }
    if (input->key_up) {
        if (menu->selected_level <= 119) menu->selected_level += 10;
        else menu->selected_level = 129;
        bal_sfx_play(5);
        menu_load_preview(menu, menu->selected_level);
    }
    if (input->key_space || input->key_1) {
        if (out_level) *out_level = menu->selected_level;
        bal_sfx_play(1);
        return MENU_ACTION_START_LEVEL;
    }

    // Handle Mouse Clicks
    if (input->mouse_left_clicked) {
        if (hover == 1) { // START
            bal_sfx_play(1);
            if (out_level) *out_level = menu->selected_level;
            return MENU_ACTION_START_LEVEL;
        } else if (hover == 2) { // PREV <
            if (menu->selected_level > 1) {
                menu->selected_level--;
                bal_sfx_play(5);
                menu_load_preview(menu, menu->selected_level);
            }
        } else if (hover == 3) { // NEXT >
            if (menu->selected_level < 129) {
                menu->selected_level++;
                bal_sfx_play(5);
                menu_load_preview(menu, menu->selected_level);
            }
        } else if (hover == 5) { // -10
            if (menu->selected_level > 10) menu->selected_level -= 10;
            else menu->selected_level = 1;
            bal_sfx_play(5);
            menu_load_preview(menu, menu->selected_level);
        } else if (hover == 6) { // +10
            if (menu->selected_level <= 119) menu->selected_level += 10;
            else menu->selected_level = 129;
            bal_sfx_play(5);
            menu_load_preview(menu, menu->selected_level);
        } else if (hover == 4) { // QUIT
            return MENU_ACTION_QUIT;
        }
    }

    return MENU_ACTION_NONE;
}

void menu_render(menu_state_t *menu, surface_t *dest) {
    if (!menu || !dest) return;

    int sw = (int)dest->width;
    int sh = (int)dest->height;

    // 1. Draw Title Screen Background (Centered)
    if (menu->title_pixels) {
        int dest_x = (sw - 640) / 2;
        int dest_y = (sh - 480) / 2;
        if (dest_x < 0) dest_x = 0;
        if (dest_y < 0) dest_y = 0;

        surface_blit_paletted_sub(dest, menu->title_pixels, 640, 480, 0, 0, dest_x, dest_y, 640, 480, &menu->palette, -1);
    }

    // 2. Main Dialog Frame (Centered at bottom: 580x202, Y=262)
    int cx = sw / 2;
    int card_x = cx - 290;
    int card_y = 262;
    int card_w = 580;
    int card_h = 202;

    // Outer shadow & multi-layer golden filigree border
    surface_fill_rect(dest, card_x - 3, card_y - 3, card_w + 6, card_h + 6, 0xAA050201);
    surface_draw_rect(dest, card_x - 2, card_y - 2, card_w + 4, card_h + 4, 0xFF5C3A1E); // Bronze border
    surface_draw_rect(dest, card_x - 1, card_y - 1, card_w + 2, card_h + 2, 0xFFFFD700); // Radiant Gold border
    surface_draw_rect(dest, card_x,     card_y,     card_w,     card_h,     0xFF8B5A2B); // Inset Copper border

    // Card background body (translucent dark mahogany)
    surface_fill_rect(dest, card_x + 1, card_y + 1, card_w - 2, card_h - 2, 0xEE160C08);

    // Subtle divider line between left preview and right controls
    surface_fill_rect(dest, card_x + 188, card_y + 12, 1, card_h - 24, 0xFF4A3420);
    surface_fill_rect(dest, card_x + 189, card_y + 12, 1, card_h - 24, 0xFF1A1008);

    // ==========================================
    // 3. LEFT PANEL: MAP PREVIEW & MISSION INTEL
    // ==========================================
    // Map Preview Title
    surface_draw_text_shadow(dest, card_x + 52, card_y + 10, "MAP PREVIEW", 0xFFFFD700, 0xFF000000, 1);

    // Beveled frame for 120x120 preview at (card_x + 24, card_y + 24)
    int pv_frame_x = card_x + 24;
    int pv_frame_y = card_y + 24;
    surface_fill_rect(dest, pv_frame_x - 2, pv_frame_y - 2, 124, 124, 0xFF000000);
    surface_draw_rect(dest, pv_frame_x - 2, pv_frame_y - 2, 124, 124, 0xFF6B4E31);
    surface_draw_rect(dest, pv_frame_x - 1, pv_frame_y - 1, 122, 122, 0xFFD4AF37);

    if (menu->preview_surf) {
        // Blit 120x120 preview map
        for (int py = 0; py < 120; py++) {
            uint32_t *drow = &dest->pixels[(pv_frame_y + py) * dest->width + pv_frame_x];
            const uint32_t *srow = &menu->preview_surf->pixels[py * 120];
            memcpy(drow, srow, 120 * sizeof(uint32_t));
        }

        // Draw Player & Enemy Base Indicators on the preview map
        int ox = (120 - (int)menu->preview_w) / 2;
        int oy = (120 - (int)menu->preview_h) / 2;

        bool blink = ((menu->anim_tick / 15) % 2 == 0);

        // Player Base marker (Bright Blue & White)
        int px = pv_frame_x + ox + menu->p_base_x;
        int py = pv_frame_y + oy + menu->p_base_y;
        if (px >= pv_frame_x && px <= pv_frame_x + 116 && py >= pv_frame_y && py <= pv_frame_y + 116) {
            surface_fill_rect(dest, px - 2, py - 2, 5, 5, 0xFF000000);
            surface_fill_rect(dest, px - 1, py - 1, 3, 3, blink ? 0xFF00FFFF : 0xFF2288FF);
            surface_fill_rect(dest, px, py, 1, 1, 0xFFFFFFFF);
        }

        // Enemy Base marker (Bright Red & Yellow)
        int ex = pv_frame_x + ox + menu->e_base_x;
        int ey = pv_frame_y + oy + menu->e_base_y;
        if (ex >= pv_frame_x && ex <= pv_frame_x + 116 && ey >= pv_frame_y && ey <= pv_frame_y + 116) {
            surface_fill_rect(dest, ex - 2, ey - 2, 5, 5, 0xFF000000);
            surface_fill_rect(dest, ex - 1, ey - 1, 3, 3, blink ? 0xFFFFEE00 : 0xFFFF2222);
            surface_fill_rect(dest, ex, ey, 1, 1, 0xFFFFFFFF);
        }
    }

    // Legend dots below preview (Y = 152 in card -> card_y + 152 = 414)
    surface_fill_rect(dest, card_x + 28, card_y + 152, 6, 6, 0xFF00FFFF);
    surface_draw_text(dest, card_x + 38, card_y + 152, "YOU", 0xFF66CCFF, 1);

    surface_fill_rect(dest, card_x + 104, card_y + 152, 6, 6, 0xFFFF2222);
    surface_draw_text(dest, card_x + 114, card_y + 152, "ENEMY", 0xFFFF6666, 1);

    // Map Name and Theme details (Y = card_y + 166 and +180)
    char map_info[48];
    snprintf(map_info, sizeof(map_info), "MAP: %s", menu->preview_name[0] ? menu->preview_name : "UNKNOWN");
    surface_draw_text_shadow(dest, card_x + 16, card_y + 166, map_info, 0xFFFFD700, 0xFF000000, 1);

    char theme_info[48];
    snprintf(theme_info, sizeof(theme_info), "%s (%ux%u)", menu->preview_theme, menu->preview_w, menu->preview_h);
    surface_draw_text_shadow(dest, card_x + 16, card_y + 180, theme_info, 0xFFCCCCCC, 0xFF000000, 1);

    // ==========================================
    // 4. RIGHT PANEL: CONTROLS & LEVEL SELECTOR
    // ==========================================
    // Top banner
    surface_draw_text_shadow(dest, card_x + 265, card_y + 14, "===  MISSION  SELECT  ===", 0xFFFFD700, 0xFF000000, 1);

    // --- LEVEL SELECTOR ROW (Y = card_y + 42 = 304) ---
    int row_y = card_y + 42;

    // Button 5: [ -10 ]
    bool h_p10 = (menu->hovered_button == 5);
    bool d_p10 = (h_p10 && menu->mouse_down);
    draw_beveled_box(dest, card_x + 215 + (d_p10?1:0), row_y + (d_p10?1:0), 42, 34,
                     h_p10 ? 0xFF4A301C : 0xFF352012,
                     d_p10 ? 0xFF1A0A04 : 0xFF8A623A,
                     d_p10 ? 0xFF8A623A : 0xFF1A0A04);
    if (h_p10) surface_draw_rect(dest, card_x + 214, row_y - 1, 44, 36, 0xFFFFD700);
    surface_draw_text_shadow(dest, card_x + 224 + (d_p10?1:0), row_y + 13 + (d_p10?1:0), "-10", 0xFFFFFFFF, 0xFF000000, 1);

    // Button 2: [ < ] (PREV)
    bool h_prv = (menu->hovered_button == 2);
    bool d_prv = (h_prv && menu->mouse_down);
    draw_beveled_box(dest, card_x + 262 + (d_prv?1:0), row_y + (d_prv?1:0), 42, 34,
                     h_prv ? 0xFF4A301C : 0xFF352012,
                     d_prv ? 0xFF1A0A04 : 0xFF8A623A,
                     d_prv ? 0xFF8A623A : 0xFF1A0A04);
    if (h_prv) surface_draw_rect(dest, card_x + 261, row_y - 1, 44, 36, 0xFFFFD700);
    surface_draw_text_shadow(dest, card_x + 279 + (d_prv?1:0), row_y + 10 + (d_prv?1:0), "<", 0xFFFFFFFF, 0xFF000000, 2);

    // Level Number Display Inset Box
    draw_beveled_box(dest, card_x + 309, row_y, 120, 34,
                     0xFF0C0806, 0xFF100804, 0xFF4A382A);
    surface_draw_rect(dest, card_x + 309, row_y, 120, 34, 0xFF8B5A2B);

    char lvl_str[32];
    snprintf(lvl_str, sizeof(lvl_str), "LEVEL %03u", menu->selected_level);
    surface_draw_text_shadow(dest, card_x + 316, row_y + 10, lvl_str, 0xFFFFD700, 0xFF000000, 1);

    // Button 3: [ > ] (NEXT)
    bool h_nxt = (menu->hovered_button == 3);
    bool d_nxt = (h_nxt && menu->mouse_down);
    draw_beveled_box(dest, card_x + 434 + (d_nxt?1:0), row_y + (d_nxt?1:0), 42, 34,
                     h_nxt ? 0xFF4A301C : 0xFF352012,
                     d_nxt ? 0xFF1A0A04 : 0xFF8A623A,
                     d_nxt ? 0xFF8A623A : 0xFF1A0A04);
    if (h_nxt) surface_draw_rect(dest, card_x + 433, row_y - 1, 44, 36, 0xFFFFD700);
    surface_draw_text_shadow(dest, card_x + 451 + (d_nxt?1:0), row_y + 10 + (d_nxt?1:0), ">", 0xFFFFFFFF, 0xFF000000, 2);

    // Button 6: [ +10 ]
    bool h_n10 = (menu->hovered_button == 6);
    bool d_n10 = (h_n10 && menu->mouse_down);
    draw_beveled_box(dest, card_x + 481 + (d_n10?1:0), row_y + (d_n10?1:0), 42, 34,
                     h_n10 ? 0xFF4A301C : 0xFF352012,
                     d_n10 ? 0xFF1A0A04 : 0xFF8A623A,
                     d_n10 ? 0xFF8A623A : 0xFF1A0A04);
    if (h_n10) surface_draw_rect(dest, card_x + 480, row_y - 1, 44, 36, 0xFFFFD700);
    surface_draw_text_shadow(dest, card_x + 490 + (d_n10?1:0), row_y + 13 + (d_n10?1:0), "+10", 0xFFFFFFFF, 0xFF000000, 1);

    // --- BUTTON 1: [ START MISSION ] (Y = card_y + 90 = 352, W=308, H=46) ---
    int btn_start_y = card_y + 90;
    bool h_start = (menu->hovered_button == 1);
    bool d_start = (h_start && menu->mouse_down);

    uint32_t start_bg = h_start ? 0xFF2A7A20 : 0xFF1E5818;
    draw_beveled_box(dest, card_x + 215 + (d_start?1:0), btn_start_y + (d_start?1:0), 308, 46,
                     start_bg,
                     d_start ? 0xFF0F300C : 0xFF66DD44,
                     d_start ? 0xFF66DD44 : 0xFF0F300C);

    if (h_start) {
        surface_draw_rect(dest, card_x + 213, btn_start_y - 2, 312, 50, 0xFFFFD700);
        surface_draw_rect(dest, card_x + 214, btn_start_y - 1, 310, 48, 0xFF44FF44);
    } else {
        surface_draw_rect(dest, card_x + 214, btn_start_y - 1, 310, 48, 0xFFB8860B);
    }

    surface_draw_text_shadow(dest, card_x + 276 + (d_start?1:0), btn_start_y + 16 + (d_start?1:0),
                             ">> START MISSION <<", 0xFFFFFFFF, 0xFF000000, 1);

    // --- BUTTON 4: [ QUIT GAME ] (Y = card_y + 146 = 408, W=308, H=36) ---
    int btn_quit_y = card_y + 146;
    bool h_quit = (menu->hovered_button == 4);
    bool d_quit = (h_quit && menu->mouse_down);

    uint32_t quit_bg = h_quit ? 0xFF7A1C1C : 0xFF501414;
    draw_beveled_box(dest, card_x + 215 + (d_quit?1:0), btn_quit_y + (d_quit?1:0), 308, 36,
                     quit_bg,
                     d_quit ? 0xFF250505 : 0xFFD84A4A,
                     d_quit ? 0xFFD84A4A : 0xFF250505);

    if (h_quit) {
        surface_draw_rect(dest, card_x + 213, btn_quit_y - 2, 312, 40, 0xFFFF4444);
    } else {
        surface_draw_rect(dest, card_x + 214, btn_quit_y - 1, 310, 38, 0xFF7A2020);
    }

    surface_draw_text_shadow(dest, card_x + 325 + (d_quit?1:0), btn_quit_y + 14 + (d_quit?1:0),
                             "QUIT GAME", 0xFFFFAAAA, 0xFF000000, 1);
}
