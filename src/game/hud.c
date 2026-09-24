#include "hud.h"
#include "assets/asset_path.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

bool hud_init(hud_resources_t *hud_res) {
    if (!hud_res) return false;
    hud_res->hpan_data = NULL;
    hud_res->hpan_size = 0;

    FILE *f = asset_open_file("BALS/HPAN640.BAL", "rb");
    if (f) {
        fseek(f, 0, SEEK_END);
        long sz = ftell(f);
        fseek(f, 0, SEEK_SET);
        if (sz > 0) {
            hud_res->hpan_data = (uint8_t*)malloc(sz);
            if (hud_res->hpan_data) {
                hud_res->hpan_size = (uint32_t)fread(hud_res->hpan_data, 1, sz, f);
            }
        }
        fclose(f);
        if (hud_res->hpan_data) return true;
    }
    return false;
}

void hud_free(hud_resources_t *hud_res) {
    if (hud_res && hud_res->hpan_data) {
        free(hud_res->hpan_data);
        hud_res->hpan_data = NULL;
        hud_res->hpan_size = 0;
    }
}

// 5x7 mini font for HUD labels
static const uint8_t g_digits[10][5] = {
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

static void draw_number_scaled(surface_t *dest, int x, int y, uint32_t num, uint32_t color, int scale) {
    char buf[16];
    snprintf(buf, sizeof(buf), "%u", num);
    int cx = x;

    for (int i = 0; buf[i] != '\0'; i++) {
        int d = buf[i] - '0';
        if (d >= 0 && d <= 9) {
            for (int r = 0; r < 5; r++) {
                uint8_t row = g_digits[d][r];
                for (int c = 0; c < 3; c++) {
                    if (row & (1 << (2 - c))) {
                        surface_fill_rect(dest, cx + (c * scale), y + (r * scale), scale, scale, color);
                    }
                }
            }
        }
        cx += (4 * scale);
    }
}

static void draw_hud_button(surface_t *dest, int x, int y, int w, int h, 
                            uint32_t role_color, uint32_t count, 
                            bool is_selected, const char *role_label) {
    (void)role_label;
    // Button background
    uint32_t bg = is_selected ? 0xFF553322 : 0xFF2A1810;
    surface_fill_rect(dest, x, y, w, h, bg);

    // Bevel borders
    uint32_t border_col = is_selected ? 0xFFFFD700 : 0xFF8B5A2B;
    surface_draw_rect(dest, x, y, w, h, border_col);
    if (is_selected) {
        surface_draw_rect(dest, x + 1, y + 1, w - 2, h - 2, 0xFFFFFFAA);
    }

    // Role Color Swatch (Icon)
    surface_fill_rect(dest, x + 8, y + 8, 20, 20, role_color);
    surface_draw_rect(dest, x + 8, y + 8, 20, 20, 0xFF000000);

    // Draw Count
    draw_number_scaled(dest, x + 36, y + 10, count, 0xFFFFFFFF, 3);
}

void hud_render(surface_t *dest,
                const hud_resources_t *hud_res,
                const hud_state_t *hud_state,
                const char *level_name,
                const bal_palette_t *palette) {
    if (!dest || !hud_state) return;
    (void)hud_res;
    (void)palette;

    int hud_h = HUD_HEIGHT;
    int hud_y = (int)dest->height - hud_h;
    if (hud_y < 0) return;

    // 1. Draw Master Bar Background
    surface_fill_rect(dest, 0, hud_y, (int)dest->width, hud_h, 0xFF1E110A); // Dark wood
    surface_fill_rect(dest, 0, hud_y, (int)dest->width, 3, 0xFFDAA520);    // Gold top trim
    surface_fill_rect(dest, 0, hud_y + 3, (int)dest->width, 2, 0xFF8B5A2B);// Shadow trim

    // 2. Render 4 Role Selection Cards
    int btn_w = 90;
    int btn_h = 36;
    int btn_y = hud_y + 7;
    int start_x = 20;
    int spacing = 100;

    // [1] Worker (Red)
    draw_hud_button(dest, start_x + 0 * spacing, btn_y, btn_w, btn_h, 
                    0xFFFF3333, hud_state->workers_red, (hud_state->selected_role == 0), "WORKER");

    // [2] Builder (Blue)
    draw_hud_button(dest, start_x + 1 * spacing, btn_y, btn_w, btn_h, 
                    0xFF3388FF, hud_state->builders_blue, (hud_state->selected_role == 1), "BUILDER");

    // [3] Scientist (White)
    draw_hud_button(dest, start_x + 2 * spacing, btn_y, btn_w, btn_h, 
                    0xFFEEEEEE, hud_state->scientists_white, (hud_state->selected_role == 2), "SCIENCE");

    // [4] Soldier (Green)
    draw_hud_button(dest, start_x + 3 * spacing, btn_y, btn_w, btn_h, 
                    0xFF22DD22, hud_state->soldiers_green, (hud_state->selected_role == 3), "SOLDIER");

    // Total Population Count
    uint32_t total = hud_state->workers_red + hud_state->builders_blue + 
                     hud_state->scientists_white + hud_state->soldiers_green;
    int pop_x = start_x + 4 * spacing + 10;
    surface_fill_rect(dest, pop_x, btn_y, 70, btn_h, 0xFF2A1810);
    surface_draw_rect(dest, pop_x, btn_y, 70, btn_h, 0xFF8B5A2B);
    draw_number_scaled(dest, pop_x + 12, btn_y + 10, total, 0xFFFFCC00, 3);

    // Level Title on far right
    (void)level_name;
}

int hud_handle_click(int mouse_x, int mouse_y, int screen_w, int screen_h) {
    (void)screen_w;
    int hud_y = screen_h - HUD_HEIGHT;
    if (mouse_y < hud_y || mouse_y > screen_h) return -1;

    int btn_w = 90;
    int btn_h = 36;
    int btn_y = hud_y + 7;
    int start_x = 20;
    int spacing = 100;

    for (int i = 0; i < 4; i++) {
        int bx = start_x + i * spacing;
        if (mouse_x >= bx && mouse_x <= (bx + btn_w) && mouse_y >= btn_y && mouse_y <= (btn_y + btn_h)) {
            return i;
        }
    }
    return -1;
}
