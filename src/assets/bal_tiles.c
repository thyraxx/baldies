#include "bal_tiles.h"
#include "asset_path.h"
#include <stdio.h>
#include <stdlib.h>

bool bal_tileset_load(const char *filepath, bal_tileset_t *out_tileset) {
    if (!filepath || !out_tileset) {
        return false;
    }

    out_tileset->data = NULL;
    out_tileset->num_tiles = 0;

    FILE *f = asset_open_file(filepath, "rb");
    if (!f) {
        return false;
    }

    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);

    if (size <= 0 || (size % TILE_PIXELS) != 0) {
        fclose(f);
        return false;
    }

    uint8_t *buffer = (uint8_t*)malloc(size);
    if (!buffer) {
        fclose(f);
        return false;
    }

    size_t read_bytes = fread(buffer, 1, size, f);
    fclose(f);

    if (read_bytes != (size_t)size) {
        free(buffer);
        return false;
    }

    out_tileset->data = buffer;
    out_tileset->num_tiles = (uint32_t)(size / TILE_PIXELS);

    // Classify water tiles
    memset(out_tileset->is_water, 0, sizeof(out_tileset->is_water));
    for (uint32_t t = 0; t < out_tileset->num_tiles && t < 1280; t++) {
        if (t >= 340 && t <= 359) {
            out_tileset->is_water[t] = 1;
            continue;
        }
        int water_count = 0;
        const uint8_t *tdata = &buffer[t * TILE_PIXELS];
        for (int p = 0; p < TILE_PIXELS; p++) {
            uint8_t c = tdata[p];
            if (c >= 38 && c <= 46) {
                water_count++;
            }
        }
        out_tileset->is_water[t] = (water_count >= 120) ? 1 : 0;
    }

    return true;
}


void bal_tileset_free(bal_tileset_t *tileset) {
    if (tileset && tileset->data) {
        free(tileset->data);
        tileset->data = NULL;
        tileset->num_tiles = 0;
    }
}

const uint8_t* bal_tileset_get_tile(const bal_tileset_t *tileset, uint32_t tile_index) {
    if (!tileset || !tileset->data || tile_index >= tileset->num_tiles) {
        return NULL;
    }
    return &tileset->data[tile_index * TILE_PIXELS];
}
