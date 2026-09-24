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

    // Classify tile passability
    memset(out_tileset->is_water, 0, sizeof(out_tileset->is_water));
    memset(out_tileset->passability, TILE_PASS_WALKABLE, sizeof(out_tileset->passability));

    for (uint32_t t = 0; t < out_tileset->num_tiles && t < 1280; t++) {
        // Pure ocean water animation tiles
        if (t >= 340 && t <= 359) {
            out_tileset->is_water[t] = 1;
            out_tileset->passability[t] = TILE_PASS_WATER;
            continue;
        }

        // Doors and entrances (walkable)
        if (t == 415 || t == 1214) {
            out_tileset->passability[t] = TILE_PASS_DOOR;
            continue;
        }

        // Player cottage solid walls and roof
        if ((t >= 374 && t <= 376) || (t >= 394 && t <= 396) || t == 414 || t == 416) {
            out_tileset->passability[t] = TILE_PASS_SOLID;
            continue;
        }

        // Enemy hut solid walls and roof
        if ((t >= 1210 && t <= 1213) || t == 1215) {
            out_tileset->passability[t] = TILE_PASS_SOLID;
            continue;
        }

        // Stones, monoliths, boulders, fallen tree logs, stumps (1180 to 1220)
        if (t >= 1180 && t <= 1220) {
            out_tileset->passability[t] = TILE_PASS_SOLID;
            continue;
        }

        // Water pixel count check
        int water_count = 0;
        const uint8_t *tdata = &buffer[t * TILE_PIXELS];
        for (int p = 0; p < TILE_PIXELS; p++) {
            uint8_t c = tdata[p];
            if (c >= 37 && c <= 44) {
                water_count++;
            }
        }
        if (water_count >= 120) {
            out_tileset->is_water[t] = 1;
            out_tileset->passability[t] = TILE_PASS_WATER;
        }
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
