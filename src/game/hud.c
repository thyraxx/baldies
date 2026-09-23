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

// Simple 3x5 bitmap font for rendering numbers in the HUD
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

static void draw_number(surface_t *dest, int x, int y, uint32_t num, uint32_t color) {
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
                        int px = cx + c;
                        int py = y + r;
                        if (px >= 0 && px < (int)dest->width && py >= 0 && py < (int)dest->height) {
                            dest->pixels[py * dest->width + px] = color;
                        }
                    }
                }
            }
        }
        cx += 4; // 3 width + 1 spacing
    }
}

void hud_render(surface_t *dest,
                const hud_resources_t *hud_res,
                const hud_state_t *hud_state,
                const bal_palette_t *palette) {
    if (!dest || !hud_state) return;

    int hud_h = 32;
    int hud_y = (int)dest->height - hud_h;
    if (hud_y < 0) return;

    // 1. Draw HUD Background (tiled 640x32 frame 0 across full width)
    if (hud_res && hud_res->hpan_data && hud_res->hpan_size >= (640 * 32) && palette) {
        const uint8_t *frame0 = hud_res->hpan_data;
        for (int x = 0; x < (int)dest->width; x += 640) {
            int chunk_w = ((int)dest->width - x < 640) ? ((int)dest->width - x) : 640;
            surface_blit_paletted_sub(dest, frame0, 640, 32, 0, 0, x, hud_y, chunk_w, 32, palette, -1);
        }
    } else {
        // Fallback procedural panel
        surface_fill_rect(dest, 0, hud_y, (int)dest->width, hud_h, 0xFF222222);
        surface_draw_rect(dest, 0, hud_y, (int)dest->width, hud_h, 0xFF888888);
    }

    // 2. Role indicators & counts
    // Role 0: Red (Workers)
    int rx = 40;
    surface_fill_rect(dest, rx, hud_y + 8, 16, 16, 0xFFFF2222);
    draw_number(dest, rx + 20, hud_y + 14, hud_state->workers_red, 0xFFFFFFFF);

    // Role 1: Blue (Builders)
    int bx = 140;
    surface_fill_rect(dest, bx, hud_y + 8, 16, 16, 0xFF2266FF);
    draw_number(dest, bx + 20, hud_y + 14, hud_state->builders_blue, 0xFFFFFFFF);

    // Role 2: White (Scientists)
    int wx = 240;
    surface_fill_rect(dest, wx, hud_y + 8, 16, 16, 0xFFEEEEEE);
    draw_number(dest, wx + 20, hud_y + 14, hud_state->scientists_white, 0xFFFFFFFF);

    // Role 3: Green (Soldiers)
    int gx = 340;
    surface_fill_rect(dest, gx, hud_y + 8, 16, 16, 0xFF22CC22);
    draw_number(dest, gx + 20, hud_y + 14, hud_state->soldiers_green, 0xFFFFFFFF);

    // Highlight selected role
    int sel_x = rx;
    if (hud_state->selected_role == 1) sel_x = bx;
    else if (hud_state->selected_role == 2) sel_x = wx;
    else if (hud_state->selected_role == 3) sel_x = gx;
    surface_draw_rect(dest, sel_x - 2, hud_y + 6, 20, 20, 0xFFFFFF00); // Yellow selection border
}
