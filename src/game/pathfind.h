#ifndef PATHFIND_H
#define PATHFIND_H

#include <stdint.h>
#include <stdbool.h>
#include "assets/bal_map.h"
#include "assets/bal_tiles.h"
#include "house.h"

#define MAX_PATH_NODES 128

typedef struct {
    float x;
    float y;
} path_point_t;

typedef struct {
    path_point_t points[MAX_PATH_NODES];
    int count;
} path_result_t;

// Checks whether tile (tx, ty) is walkable
bool pathfind_is_tile_walkable(const bal_map_t *map, const bal_tileset_t *tileset, const house_manager_t *houses, int tx, int ty);

// Finds nearest walkable tile to (target_tx, target_ty) if it is solid (searches outward up to radius 6)
bool pathfind_find_nearest_walkable_tile(const bal_map_t *map, const bal_tileset_t *tileset, const house_manager_t *houses, int target_tx, int target_ty, int *out_tx, int *out_ty);

// Finds an A* path from (start_x, start_y) to (target_x, target_y)
// Populates out_path with smoothed waypoints. Returns true if a path was found.
bool pathfind_find_path(const bal_map_t *map, const bal_tileset_t *tileset, const house_manager_t *houses,
                        float start_x, float start_y, float target_x, float target_y,
                        path_result_t *out_path);

#endif // PATHFIND_H
