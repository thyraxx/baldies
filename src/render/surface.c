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
