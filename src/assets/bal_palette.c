#include "bal_palette.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

bool bal_palette_load_from_memory(const uint8_t *data, size_t size, bal_palette_t *out_palette) {
    if (!data || size < 768 || !out_palette) {
        return false;
    }

    for (int i = 0; i < 256; i++) {
        uint8_t r = data[i * 3 + 0];
        uint8_t g = data[i * 3 + 1];
        uint8_t b = data[i * 3 + 2];

        out_palette->colors[i].r = r;
        out_palette->colors[i].g = g;
        out_palette->colors[i].b = b;

        // 0xFFRRGGBB (ARGB)
        out_palette->argb[i] = ((uint32_t)0xFF << 24) |
                               ((uint32_t)r << 16) |
                               ((uint32_t)g << 8)  |
                               ((uint32_t)b);

        // 0xFFBBGGRR (BGRA for Win32 Little-Endian DIB 0x00RRGGBB in memory)
        out_palette->bgra[i] = ((uint32_t)0xFF << 24) |
                               ((uint32_t)b << 16) |
                               ((uint32_t)g << 8)  |
                               ((uint32_t)r);
    }

    return true;
}

#include "asset_path.h"

bool bal_palette_load(const char *filepath, bal_palette_t *out_palette) {
    if (!filepath || !out_palette) {
        return false;
    }

    FILE *f = asset_open_file(filepath, "rb");
    if (!f) {
        return false;
    }

    uint8_t buffer[768];
    size_t read_bytes = fread(buffer, 1, 768, f);
    fclose(f);

    if (read_bytes < 768) {
        return false;
    }

    return bal_palette_load_from_memory(buffer, 768, out_palette);
}
