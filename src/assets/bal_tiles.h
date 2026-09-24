#ifndef BAL_TILES_H
#define BAL_TILES_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#define TILE_WIDTH  16
#define TILE_HEIGHT 16
#define TILE_PIXELS (TILE_WIDTH * TILE_HEIGHT) // 256 bytes

typedef struct {
    uint8_t *data;          // Array of tile pixel data (num_tiles * 256 bytes)
    uint32_t num_tiles;     // Number of 16x16 tiles (typically 1280)
    uint8_t is_water[1280]; // 1 if water, 0 if land
} bal_tileset_t;



bool bal_tileset_load(const char *filepath, bal_tileset_t *out_tileset);
void bal_tileset_free(bal_tileset_t *tileset);
const uint8_t* bal_tileset_get_tile(const bal_tileset_t *tileset, uint32_t tile_index);

#endif // BAL_TILES_H
