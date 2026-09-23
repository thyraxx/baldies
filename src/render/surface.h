#ifndef SURFACE_H
#define SURFACE_H

#include <stdint.h>
#include <stdbool.h>
#include "assets/bal_palette.h"

typedef struct {
    uint32_t width;
    uint32_t height;
    uint32_t pitch;     // Bytes per row (width * 4)
    uint32_t *pixels;   // 32-bit BGRA format for native Win32 DIB blitting
} surface_t;

surface_t* surface_create(uint32_t width, uint32_t height);
void surface_destroy(surface_t *surface);
void surface_clear(surface_t *surface, uint32_t color);

void surface_blit_tile(surface_t *dest, 
                       const uint8_t *tile_pixels, 
                       int dest_x, int dest_y, 
                       const bal_palette_t *palette);

void surface_blit_paletted_sub(surface_t *dest,
                               const uint8_t *src_pixels,
                               int src_w, int src_h,
                               int src_x, int src_y,
                               int dest_x, int dest_y,
                               int w, int h,
                               const bal_palette_t *palette,
                               int color_key);

void surface_draw_rect(surface_t *dest, int x, int y, int w, int h, uint32_t color);
void surface_fill_rect(surface_t *dest, int x, int y, int w, int h, uint32_t color);

#endif // SURFACE_H
