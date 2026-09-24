#include "menu.h"
#include "assets/asset_path.h"
#include "assets/bal_sfx.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

bool menu_init(menu_state_t *menu) {
    if (!menu) return false;
    memset(menu, 0, sizeof(menu_state_t));
    menu->selected_level = 1;

    // Load Title Palette
    if (!bal_palette_load("BALS/BTPAL.BAL", &menu->palette)) {
        return false;
    }

    // Load Title Image (640x480)
    FILE *f = asset_open_file("BALS/BALDTITL.BAL", "rb");
    if (!f) return false;

    menu->title_pixels = (uint8_t*)malloc(640 * 480);
    if (!menu->title_pixels) {
        fclose(f);
        return false;
    }

    size_t read_bytes = fread(menu->title_pixels, 1, 640 * 480, f);
    fclose(f);

    return (read_bytes == (640 * 480));
}

void menu_free(menu_state_t *menu) {
    if (menu && menu->title_pixels) {
        free(menu->title_pixels);
        menu->title_pixels = NULL;
    }
}

static const uint8_t g_font_chars[10][5] = {
    { 0x7, 0x5, 0x5, 0x5, 0x7 }, // 0
    { 0x2, 0x6, 0x2, 0x2, 0x7 }, // 1
    { 0x7, 0x1, 0x7, 0x4, 0x7 }, // 2
    { 0x7, 0x1, 0x7, 0x1, 0x7 }, // 3
    { 0x5, 0x5, 0x7, 0x1, 0x1 }, // 4
    { 0x7, 0x4, 0x7, 0x1, 0x7 }, // 5
    { 0x7, 0x4, 0x7, 0x5, 0x7 }, // 6
    { 0x7, 0x1, 0x2, 0x2, 0x2 }, // 7
    { 0x7, 0x5, 0x7, 0x5, 0x7 }, // 8
    { 0x7, 0x5, 0x7, 0x1, 0x7 }, // 9
};

static void draw_digits(surface_t *dest, int x, int y, uint32_t val, int scale, uint32_t color) {
    char buf[16];
    snprintf(buf, sizeof(buf), "%u", val);
    int cx = x;
    for (int i = 0; buf[i] != '\0'; i++) {
        int d = buf[i] - '0';
        if (d >= 0 && d <= 9) {
            for (int r = 0; r < 5; r++) {
                uint8_t row = g_font_chars[d][r];
                for (int c = 0; c < 3; c++) {
                    if (row & (1 << (2 - c))) {
                        surface_fill_rect(dest, cx + c * scale, y + r * scale, scale, scale, color);
                    }
                }
            }
        }
        cx += (4 * scale);
    }
}

menu_action_t menu_update(menu_state_t *menu, const platform_input_t *input, int screen_w, int screen_h, uint32_t *out_level) {
    if (!menu || !input) return MENU_ACTION_NONE;

    // Keyboard Shortcuts
    if (input->key_left) {
        if (menu->selected_level > 1) {
            menu->selected_level--;
            bal_sfx_play(5);
        }
    }
    if (input->key_right) {
        if (menu->selected_level < 129) {
            menu->selected_level++;
            bal_sfx_play(5);
        }
    }
    if (input->key_space || (input->key_1 && !menu->in_level_select)) {
        if (out_level) *out_level = menu->selected_level;
        return MENU_ACTION_START_LEVEL;
    }

    // Mouse Button Clicks
    if (input->mouse_left_clicked) {
        int cx = screen_w / 2;
        int cy = screen_h / 2 + 100;

        // Button 1: [ START GAME ] (cx - 150, cy - 30, w=300, h=40)
        if (input->mouse_x >= (cx - 150) && input->mouse_x <= (cx + 150) &&
            input->mouse_y >= (cy - 30) && input->mouse_y <= (cy + 10)) {
            bal_sfx_play(1);
            if (out_level) *out_level = menu->selected_level;
            return MENU_ACTION_START_LEVEL;
        }

        // Button 2: [ < PREV LEVEL ] (cx - 150, cy + 25, w=80, h=36)
        if (input->mouse_x >= (cx - 150) && input->mouse_x <= (cx - 70) &&
            input->mouse_y >= (cy + 25) && input->mouse_y <= (cy + 61)) {
            if (menu->selected_level > 1) {
                menu->selected_level--;
                bal_sfx_play(5);
            }
        }

        // Button 3: [ NEXT LEVEL > ] (cx + 70, cy + 25, w=80, h=36)
        if (input->mouse_x >= (cx + 70) && input->mouse_x <= (cx + 150) &&
            input->mouse_y >= (cy + 25) && input->mouse_y <= (cy + 61)) {
            if (menu->selected_level < 129) {
                menu->selected_level++;
                bal_sfx_play(5);
            }
        }

        // Button 4: [ QUIT ] (cx - 150, cy + 80, w=300, h=36)
        if (input->mouse_x >= (cx - 150) && input->mouse_x <= (cx + 150) &&
            input->mouse_y >= (cy + 80) && input->mouse_y <= (cy + 116)) {
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

    // 2. Draw Menu Card
    int cx = sw / 2;
    int cy = sh / 2 + 100;
    int card_w = 340;
    int card_h = 160;

    surface_fill_rect(dest, cx - (card_w / 2), cy - 40, card_w, card_h, 0xDD1A0E07); // Dark translucent wood
    surface_draw_rect(dest, cx - (card_w / 2), cy - 40, card_w, card_h, 0xFFDAA520); // Gold border
    surface_draw_rect(dest, cx - (card_w / 2) + 2, cy - 38, card_w - 4, card_h - 4, 0xFF8B5A2B);

    // Button 1: [ START GAME ]
    surface_fill_rect(dest, cx - 140, cy - 25, 280, 36, 0xFF2A5522); // Green banner
    surface_draw_rect(dest, cx - 140, cy - 25, 280, 36, 0xFF44FF44);
    // Draw Level number
    draw_digits(dest, cx - 35, cy - 18, menu->selected_level, 4, 0xFFFFFFFF);

    // Button 2: [ < PREV ]
    surface_fill_rect(dest, cx - 140, cy + 25, 80, 32, 0xFF3D2314);
    surface_draw_rect(dest, cx - 140, cy + 25, 80, 32, 0xFFDAA520);
    // Left arrow triangle / dash
    surface_fill_rect(dest, cx - 105, cy + 39, 12, 4, 0xFFFFFFFF);

    // Level Display in Middle
    surface_fill_rect(dest, cx - 45, cy + 25, 90, 32, 0xFF111111);
    surface_draw_rect(dest, cx - 45, cy + 25, 90, 32, 0xFF888888);
    draw_digits(dest, cx - 15, cy + 31, menu->selected_level, 4, 0xFFFFCC00);

    // Button 3: [ NEXT > ]
    surface_fill_rect(dest, cx + 60, cy + 25, 80, 32, 0xFF3D2314);
    surface_draw_rect(dest, cx + 60, cy + 25, 80, 32, 0xFFDAA520);
    // Right arrow dash
    surface_fill_rect(dest, cx + 95, cy + 39, 12, 4, 0xFFFFFFFF);

    // Button 4: [ QUIT ]
    surface_fill_rect(dest, cx - 140, cy + 72, 280, 32, 0xFF552222);
    surface_draw_rect(dest, cx - 140, cy + 72, 280, 32, 0xFFFF6666);
}
