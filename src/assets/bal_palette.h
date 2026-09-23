#ifndef BAL_PALETTE_H
#define BAL_PALETTE_H

#include <stdint.h>
#include <stdbool.h>

typedef struct {
    uint8_t r;
    uint8_t g;
    uint8_t b;
} bal_color_rgb_t;

typedef struct {
    bal_color_rgb_t colors[256];
    uint32_t argb[256]; // 0xAARRGGBB format for rendering
    uint32_t bgra[256]; // 0xAABBGGRR format for Win32 DIB
} bal_palette_t;

bool bal_palette_load(const char *filepath, bal_palette_t *out_palette);
bool bal_palette_load_from_memory(const uint8_t *data, size_t size, bal_palette_t *out_palette);

#endif // BAL_PALETTE_H
