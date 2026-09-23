#ifndef BAL_MAP_H
#define BAL_MAP_H

#include <stdint.h>
#include <stdbool.h>

typedef struct {
    char name[32];             // e.g. "GRASS1"
    char block_file[32];       // e.g. "LEV1BLK"
    char map_file[32];         // e.g. "MAP001"
    uint16_t width;            // Map width in tiles (e.g. 116)
    uint16_t height;           // Map height in tiles (e.g. 112)
    uint16_t start_cam_x;      // Initial camera X in pixels (e.g. 880)
    uint16_t start_cam_y;      // Initial camera Y in pixels (e.g. 688)
    uint16_t *tiles;           // Array of width * height tile indices
    uint32_t total_tiles;      // width * height
} bal_map_t;

bool bal_map_load(uint32_t level_num, bal_map_t *out_map);
void bal_map_free(bal_map_t *map);
uint16_t bal_map_get_tile(const bal_map_t *map, uint32_t tile_x, uint32_t tile_y);

#endif // BAL_MAP_H
