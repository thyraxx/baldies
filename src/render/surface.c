#include "surface.h"
#include <stdlib.h>
#include <string.h>

surface_t* surface_create(uint32_t width, uint32_t height) {
    if (width == 0 || height == 0) return NULL;

    surface_t *s = (surface_t*)malloc(sizeof(surface_t));
    if (!s) return NULL;

    s->width = width;
    s->height = height;
    s->pitch = width * sizeof(uint32_t);
    s->pixels = (uint32_t*)malloc(s->pitch * height);

    if (!s->pixels) {
        free(s);
        return NULL;
    }

    surface_clear(s, 0xFF000000);
    return s;
}

void surface_destroy(surface_t *surface) {
    if (surface) {
        if (surface->pixels) {
            free(surface->pixels);
        }
        free(surface);
    }
}

void surface_clear(surface_t *surface, uint32_t color) {
    if (!surface || !surface->pixels) return;
    size_t count = (size_t)surface->width * surface->height;
    for (size_t i = 0; i < count; i++) {
        surface->pixels[i] = color;
    }
}

void surface_blit_tile(surface_t *dest, 
                       const uint8_t *tile_pixels, 
                       int dest_x, int dest_y, 
                       const bal_palette_t *palette) {
    if (!dest || !dest->pixels || !tile_pixels || !palette) return;

    int dw = (int)dest->width;
    int dh = (int)dest->height;

    // Fast-path: fully on-screen 16x16 tile
    if (dest_x >= 0 && (dest_x + 16) <= dw && dest_y >= 0 && (dest_y + 16) <= dh) {
        for (int y = 0; y < 16; y++) {
            uint32_t *drow = &dest->pixels[(dest_y + y) * dw + dest_x];
            const uint8_t *srow = &tile_pixels[y * 16];
            for (int x = 0; x < 16; x++) {
                drow[x] = palette->bgra[srow[x]];
            }
        }
        return;
    }

    // Clipped tile blit
    int start_x = (dest_x < 0) ? -dest_x : 0;
    int start_y = (dest_y < 0) ? -dest_y : 0;
    int end_x = (dest_x + 16 > dw) ? (dw - dest_x) : 16;
    int end_y = (dest_y + 16 > dh) ? (dh - dest_y) : 16;

    for (int y = start_y; y < end_y; y++) {
        uint32_t *drow = &dest->pixels[(dest_y + y) * dw + dest_x];
        const uint8_t *srow = &tile_pixels[y * 16];
        for (int x = start_x; x < end_x; x++) {
            drow[x] = palette->bgra[srow[x]];
        }
    }
}


void surface_blit_paletted_sub(surface_t *dest,
                               const uint8_t *src_pixels,
                               int src_w, int src_h,
                               int src_x, int src_y,
                               int dest_x, int dest_y,
                               int w, int h,
                               const bal_palette_t *palette,
                               int color_key) {
    if (!dest || !dest->pixels || !src_pixels || !palette) return;

    int dw = (int)dest->width;
    int dh = (int)dest->height;

    for (int py = 0; py < h; py++) {
        int sy = src_y + py;
        int dy = dest_y + py;
        if (sy < 0 || sy >= src_h || dy < 0 || dy >= dh) continue;

        const uint8_t *srow = &src_pixels[sy * src_w];
        uint32_t *drow = &dest->pixels[dy * dw];

        for (int px = 0; px < w; px++) {
            int sx = src_x + px;
            int dx = dest_x + px;
            if (sx < 0 || sx >= src_w || dx < 0 || dx >= dw) continue;

            uint8_t c = srow[sx];
            if (color_key >= 0 && c == (uint8_t)color_key) continue;

            drow[dx] = palette->bgra[c];
        }
    }
}

void surface_draw_rect(surface_t *dest, int x, int y, int w, int h, uint32_t color) {
    if (!dest || !dest->pixels || w <= 0 || h <= 0) return;
    int dw = (int)dest->width;
    int dh = (int)dest->height;

    // Horizontal top & bottom
    for (int px = 0; px < w; px++) {
        int rx = x + px;
        if (rx >= 0 && rx < dw) {
            if (y >= 0 && y < dh) dest->pixels[y * dw + rx] = color;
            if ((y + h - 1) >= 0 && (y + h - 1) < dh) dest->pixels[(y + h - 1) * dw + rx] = color;
        }
    }
    // Vertical left & right
    for (int py = 0; py < h; py++) {
        int ry = y + py;
        if (ry >= 0 && ry < dh) {
            if (x >= 0 && x < dw) dest->pixels[ry * dw + x] = color;
            if ((x + w - 1) >= 0 && (x + w - 1) < dw) dest->pixels[ry * dw + (x + w - 1)] = color;
        }
    }
}

void surface_fill_rect(surface_t *dest, int x, int y, int w, int h, uint32_t color) {
    if (!dest || !dest->pixels || w <= 0 || h <= 0) return;
    int dw = (int)dest->width;
    int dh = (int)dest->height;

    int sx = (x < 0) ? 0 : x;
    int sy = (y < 0) ? 0 : y;
    int ex = (x + w > dw) ? dw : (x + w);
    int ey = (y + h > dh) ? dh : (y + h);

    for (int ry = sy; ry < ey; ry++) {
        uint32_t *row = &dest->pixels[ry * dw];
        for (int rx = sx; rx < ex; rx++) {
            row[rx] = color;
        }
    }
}

// 5x7 ASCII Font Table (Chars 32 ' ' to 93 ']')
static const uint8_t g_font5x7[62][7] = {
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, // 32 ' '
    { 0x04, 0x04, 0x04, 0x04, 0x04, 0x00, 0x04 }, // 33 '!'
    { 0x0A, 0x0A, 0x0A, 0x00, 0x00, 0x00, 0x00 }, // 34 '"'
    { 0x0A, 0x0A, 0x1F, 0x0A, 0x1F, 0x0A, 0x0A }, // 35 '#'
    { 0x04, 0x0F, 0x14, 0x0E, 0x05, 0x1E, 0x04 }, // 36 '$'
    { 0x19, 0x19, 0x02, 0x04, 0x08, 0x13, 0x13 }, // 37 '%'
    { 0x0C, 0x12, 0x14, 0x08, 0x15, 0x12, 0x0D }, // 38 '&'
    { 0x04, 0x04, 0x08, 0x00, 0x00, 0x00, 0x00 }, // 39 '\''
    { 0x02, 0x04, 0x08, 0x08, 0x08, 0x04, 0x02 }, // 40 '('
    { 0x08, 0x04, 0x02, 0x02, 0x02, 0x04, 0x08 }, // 41 ')'
    { 0x00, 0x04, 0x15, 0x0E, 0x15, 0x04, 0x00 }, // 42 '*'
    { 0x00, 0x04, 0x04, 0x1F, 0x04, 0x04, 0x00 }, // 43 '+'
    { 0x00, 0x00, 0x00, 0x00, 0x04, 0x04, 0x08 }, // 44 ','
    { 0x00, 0x00, 0x00, 0x1F, 0x00, 0x00, 0x00 }, // 45 '-'
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x06, 0x06 }, // 46 '.'
    { 0x01, 0x02, 0x04, 0x08, 0x10, 0x00, 0x00 }, // 47 '/'
    { 0x0E, 0x11, 0x13, 0x15, 0x19, 0x11, 0x0E }, // 48 '0'
    { 0x04, 0x0C, 0x04, 0x04, 0x04, 0x04, 0x0E }, // 49 '1'
    { 0x0E, 0x11, 0x01, 0x02, 0x04, 0x08, 0x1F }, // 50 '2'
    { 0x1F, 0x02, 0x04, 0x02, 0x01, 0x11, 0x0E }, // 51 '3'
    { 0x02, 0x06, 0x0A, 0x12, 0x1F, 0x02, 0x02 }, // 52 '4'
    { 0x1F, 0x10, 0x1E, 0x01, 0x01, 0x11, 0x0E }, // 53 '5'
    { 0x06, 0x08, 0x10, 0x1E, 0x11, 0x11, 0x0E }, // 54 '6'
    { 0x1F, 0x01, 0x02, 0x04, 0x08, 0x08, 0x08 }, // 55 '7'
    { 0x0E, 0x11, 0x11, 0x0E, 0x11, 0x11, 0x0E }, // 56 '8'
    { 0x0E, 0x11, 0x11, 0x0F, 0x01, 0x02, 0x0C }, // 57 '9'
    { 0x00, 0x0C, 0x0C, 0x00, 0x0C, 0x0C, 0x00 }, // 58 ':'
    { 0x00, 0x0C, 0x0C, 0x00, 0x0C, 0x04, 0x08 }, // 59 ';'
    { 0x02, 0x04, 0x08, 0x10, 0x08, 0x04, 0x02 }, // 60 '<'
    { 0x00, 0x1F, 0x00, 0x1F, 0x00, 0x00, 0x00 }, // 61 '='
    { 0x08, 0x04, 0x02, 0x01, 0x02, 0x04, 0x08 }, // 62 '>'
    { 0x0E, 0x11, 0x01, 0x02, 0x04, 0x00, 0x04 }, // 63 '?'
    { 0x0E, 0x11, 0x01, 0x0D, 0x15, 0x15, 0x0E }, // 64 '@'
    { 0x0E, 0x11, 0x11, 0x1F, 0x11, 0x11, 0x11 }, // 65 'A'
    { 0x1E, 0x11, 0x11, 0x1E, 0x11, 0x11, 0x1E }, // 66 'B'
    { 0x0E, 0x11, 0x10, 0x10, 0x10, 0x11, 0x0E }, // 67 'C'
    { 0x1C, 0x12, 0x11, 0x11, 0x11, 0x12, 0x1C }, // 68 'D'
    { 0x1F, 0x10, 0x10, 0x1E, 0x10, 0x10, 0x1F }, // 69 'E'
    { 0x1F, 0x10, 0x10, 0x1E, 0x10, 0x10, 0x10 }, // 70 'F'
    { 0x0E, 0x11, 0x10, 0x17, 0x11, 0x11, 0x0F }, // 71 'G'
    { 0x11, 0x11, 0x11, 0x1F, 0x11, 0x11, 0x11 }, // 72 'H'
    { 0x0E, 0x04, 0x04, 0x04, 0x04, 0x04, 0x0E }, // 73 'I'
    { 0x07, 0x02, 0x02, 0x02, 0x02, 0x12, 0x0C }, // 74 'J'
    { 0x11, 0x12, 0x14, 0x18, 0x14, 0x12, 0x11 }, // 75 'K'
    { 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x1F }, // 76 'L'
    { 0x11, 0x1B, 0x15, 0x15, 0x11, 0x11, 0x11 }, // 77 'M'
    { 0x11, 0x19, 0x15, 0x13, 0x11, 0x11, 0x11 }, // 78 'N'
    { 0x0E, 0x11, 0x11, 0x11, 0x11, 0x11, 0x0E }, // 79 'O'
    { 0x1E, 0x11, 0x11, 0x1E, 0x10, 0x10, 0x10 }, // 80 'P'
    { 0x0E, 0x11, 0x11, 0x11, 0x15, 0x12, 0x0D }, // 81 'Q'
    { 0x1E, 0x11, 0x11, 0x1E, 0x14, 0x12, 0x11 }, // 82 'R'
    { 0x0E, 0x11, 0x10, 0x0E, 0x01, 0x11, 0x0E }, // 83 'S'
    { 0x1F, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04 }, // 84 'T'
    { 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x0E }, // 85 'U'
    { 0x11, 0x11, 0x11, 0x11, 0x11, 0x0A, 0x04 }, // 86 'V'
    { 0x11, 0x11, 0x11, 0x15, 0x15, 0x1B, 0x11 }, // 87 'W'
    { 0x11, 0x11, 0x0A, 0x04, 0x0A, 0x11, 0x11 }, // 88 'X'
    { 0x11, 0x11, 0x0A, 0x04, 0x04, 0x04, 0x04 }, // 89 'Y'
    { 0x1F, 0x01, 0x02, 0x04, 0x08, 0x10, 0x1F }, // 90 'Z'
    { 0x0E, 0x08, 0x08, 0x08, 0x08, 0x08, 0x0E }, // 91 '['
    { 0x10, 0x08, 0x04, 0x02, 0x01, 0x00, 0x00 }, // 92 '\'
    { 0x0E, 0x02, 0x02, 0x02, 0x02, 0x02, 0x0E }, // 93 ']'
};

void surface_draw_char(surface_t *dest, int x, int y, char c, uint32_t color, int scale) {
    if (!dest || !dest->pixels || scale <= 0) return;
    if (c >= 'a' && c <= 'z') c = (char)(c - 'a' + 'A');
    if (c < 32 || c > 93) return;

    const uint8_t *glyph = g_font5x7[c - 32];
    for (int r = 0; r < 7; r++) {
        uint8_t row = glyph[r];
        for (int col = 0; col < 5; col++) {
            if (row & (1 << (4 - col))) {
                surface_fill_rect(dest, x + col * scale, y + r * scale, scale, scale, color);
            }
        }
    }
}

void surface_draw_text(surface_t *dest, int x, int y, const char *text, uint32_t color, int scale) {
    if (!dest || !text || scale <= 0) return;
    int cx = x;
    int char_w = 6 * scale;
    for (int i = 0; text[i] != '\0'; i++) {
        surface_draw_char(dest, cx, y, text[i], color, scale);
        cx += char_w;
    }
}

void surface_draw_text_shadow(surface_t *dest, int x, int y, const char *text, uint32_t color, uint32_t shadow_color, int scale) {
    surface_draw_text(dest, x + scale, y + scale, text, shadow_color, scale);
    surface_draw_text(dest, x, y, text, color, scale);
}
