#include "bal_tiles.h"
#include <stdio.h>
#include <stdlib.h>

bool bal_tileset_load(const char *filepath, bal_tileset_t *out_tileset) {
    if (!filepath || !out_tileset) {
        return false;
    }

    out_tileset->data = NULL;
    out_tileset->num_tiles = 0;

    FILE *f = fopen(filepath, "rb");
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
