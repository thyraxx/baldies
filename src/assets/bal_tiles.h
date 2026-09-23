#ifndef BAL_TILES_H
#define BAL_TILES_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#define TILE_WIDTH  32
#define TILE_HEIGHT 32
#define TILE_PIXELS (TILE_WIDTH * TILE_HEIGHT) // 1024 bytes

typedef struct {
    uint8_t *data;         // Array of tile pixel data (num_tiles * 1024 bytes)
    uint32_t num_tiles;    // Number of 32x32 tiles (typically 320)
} bal_tileset_t;

bool bal_tileset_load(const char *filepath, bal_tileset_t *out_tileset);
void bal_tileset_free(bal_tileset_t *tileset);
const uint8_t* bal_tileset_get_tile(const bal_tileset_t *tileset, uint32_t tile_index);

#endif // BAL_TILES_H
