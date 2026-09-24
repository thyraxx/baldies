#include "pathfind.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define MAX_GRID_NODES 16384
#define HEAP_CAPACITY 2048

typedef struct {
    uint16_t parent;
    uint16_t g_cost;
    uint16_t f_cost;
    uint8_t run_id;
    uint8_t closed;
} astar_node_t;

static astar_node_t g_nodes[MAX_GRID_NODES];
static uint8_t g_search_id = 0;

static uint16_t g_heap[HEAP_CAPACITY];
static int g_heap_size = 0;

static void heap_push(uint16_t idx) {
    if (g_heap_size >= HEAP_CAPACITY - 1) return;
    int i = g_heap_size++;
    g_heap[i] = idx;
    while (i > 0) {
        int p = (i - 1) / 2;
        if (g_nodes[g_heap[p]].f_cost <= g_nodes[g_heap[i]].f_cost) break;
        uint16_t tmp = g_heap[p];
        g_heap[p] = g_heap[i];
        g_heap[i] = tmp;
        i = p;
    }
}

static uint16_t heap_pop(void) {
    if (g_heap_size <= 0) return 0xFFFF;
    uint16_t ret = g_heap[0];
    g_heap_size--;
    if (g_heap_size > 0) {
        g_heap[0] = g_heap[g_heap_size];
        int i = 0;
        while (1) {
            int left = 2 * i + 1;
            int right = 2 * i + 2;
            int smallest = i;
            if (left < g_heap_size && g_nodes[g_heap[left]].f_cost < g_nodes[g_heap[smallest]].f_cost) smallest = left;
            if (right < g_heap_size && g_nodes[g_heap[right]].f_cost < g_nodes[g_heap[smallest]].f_cost) smallest = right;
            if (smallest == i) break;
            uint16_t tmp = g_heap[i];
            g_heap[i] = g_heap[smallest];
            g_heap[smallest] = tmp;
            i = smallest;
        }
    }
    return ret;
}

bool pathfind_is_tile_walkable(const bal_map_t *map, const bal_tileset_t *tileset, const house_manager_t *houses, int tx, int ty) {
    if (!map || tx < 1 || tx >= (int)map->width - 1 || ty < 1 || ty >= (int)map->height - 1) {
        return false;
    }

    uint16_t tile = bal_map_get_tile(map, (uint32_t)tx, (uint32_t)ty);

    // 1. Water animation tiles (340 to 359)
    if (tile >= 340 && tile <= 359) {
        return false;
    }

    // 2. Tileset passability classification
    if (tileset && tile < 1280) {
        uint8_t pass = tileset->passability[tile];
        if (pass == TILE_PASS_WATER || pass == TILE_PASS_SOLID) {
            return false;
        }
    }

    // 3. Dynamic House Manager check (check center of tile)
    if (houses) {
        float cx = tx * 16.0f + 8.0f;
        float cy = ty * 16.0f + 8.0f;
        if (house_manager_is_point_blocked(houses, cx, cy)) {
            return false;
        }
    }

    return true;
}

bool pathfind_find_nearest_walkable_tile(const bal_map_t *map, const bal_tileset_t *tileset, const house_manager_t *houses, int target_tx, int target_ty, int *out_tx, int *out_ty) {
    if (!map || !out_tx || !out_ty) return false;

    if (pathfind_is_tile_walkable(map, tileset, houses, target_tx, target_ty)) {
        *out_tx = target_tx;
        *out_ty = target_ty;
        return true;
    }

    // Spiral search outward up to radius 8
    for (int r = 1; r <= 8; r++) {
        for (int dy = -r; dy <= r; dy++) {
            for (int dx = -r; dx <= r; dx++) {
                if (abs(dx) != r && abs(dy) != r) continue;
                int nx = target_tx + dx;
                int ny = target_ty + dy;
                if (pathfind_is_tile_walkable(map, tileset, houses, nx, ny)) {
                    *out_tx = nx;
                    *out_ty = ny;
                    return true;
                }
            }
        }
    }
    return false;
}

static uint16_t heuristic(int x1, int y1, int x2, int y2) {
    int dx = abs(x1 - x2);
    int dy = abs(y1 - y2);
    // Octile distance: 10 orthogonal, 14 diagonal
    return (dx > dy) ? (uint16_t)(14 * dy + 10 * (dx - dy)) : (uint16_t)(14 * dx + 10 * (dy - dx));
}

static bool is_line_walkable(const bal_map_t *map, const bal_tileset_t *tileset, const house_manager_t *houses,
                             float x0, float y0, float x1, float y1) {
    float dx = x1 - x0;
    float dy = y1 - y0;
    float dist = sqrtf(dx * dx + dy * dy);
    if (dist < 2.0f) return true;

    int steps = (int)(dist / 6.0f) + 1;
    for (int i = 0; i <= steps; i++) {
        float t = (float)i / (float)steps;
        float sx = x0 + dx * t;
        float sy = y0 + dy * t;
        int tx = (int)(sx / 16.0f);
        int ty = (int)(sy / 16.0f);
        if (!pathfind_is_tile_walkable(map, tileset, houses, tx, ty)) {
            return false;
        }
    }
    return true;
}

bool pathfind_find_path(const bal_map_t *map, const bal_tileset_t *tileset, const house_manager_t *houses,
                        float start_x, float start_y, float target_x, float target_y,
                        path_result_t *out_path) {
    if (!map || !out_path) return false;
    out_path->count = 0;

    int start_tx = (int)(start_x / 16.0f);
    int start_ty = (int)(start_y / 16.0f);
    int target_tx = (int)(target_x / 16.0f);
    int target_ty = (int)(target_y / 16.0f);

    // Resolve start tile if slightly off walkable
    if (!pathfind_is_tile_walkable(map, tileset, houses, start_tx, start_ty)) {
        if (!pathfind_find_nearest_walkable_tile(map, tileset, houses, start_tx, start_ty, &start_tx, &start_ty)) {
            return false;
        }
    }

    // Resolve target tile if solid (e.g. clicked on stone, tree log, or wall)
    bool target_was_walkable = true;
    if (!pathfind_is_tile_walkable(map, tileset, houses, target_tx, target_ty)) {
        target_was_walkable = false;
        if (!pathfind_find_nearest_walkable_tile(map, tileset, houses, target_tx, target_ty, &target_tx, &target_ty)) {
            return false;
        }
    }

    if (start_tx == target_tx && start_ty == target_ty) {
        out_path->points[0].x = target_x;
        out_path->points[0].y = target_y;
        out_path->count = 1;
        return true;
    }

    uint32_t map_w = map->width;
    uint32_t total_tiles = (uint32_t)map->width * map->height;
    if (total_tiles > MAX_GRID_NODES) total_tiles = MAX_GRID_NODES;

    // Reset search session
    g_search_id++;
    if (g_search_id == 0) {
        memset(g_nodes, 0, sizeof(g_nodes));
        g_search_id = 1;
    }
    g_heap_size = 0;

    uint16_t start_idx = (uint16_t)(start_ty * map_w + start_tx);
    uint16_t target_idx = (uint16_t)(target_ty * map_w + target_tx);

    g_nodes[start_idx].parent = 0xFFFF;
    g_nodes[start_idx].g_cost = 0;
    g_nodes[start_idx].f_cost = heuristic(start_tx, start_ty, target_tx, target_ty);
    g_nodes[start_idx].run_id = g_search_id;
    g_nodes[start_idx].closed = 0;

    heap_push(start_idx);

    // 8-directional neighbor offsets: dx, dy, cost
    static const int dirs[8][3] = {
        {  0, -1, 10 }, // N
        {  1,  0, 10 }, // E
        {  0,  1, 10 }, // S
        { -1,  0, 10 }, // W
        {  1, -1, 14 }, // NE
        {  1,  1, 14 }, // SE
        { -1,  1, 14 }, // SW
        { -1, -1, 14 }  // NW
    };

    bool reached = false;
    uint16_t closest_idx = start_idx;
    uint16_t closest_h = 0xFFFF;
    int iterations = 0;

    while (g_heap_size > 0 && iterations++ < 3000) {
        uint16_t curr_idx = heap_pop();
        if (curr_idx == 0xFFFF) break;

        if (g_nodes[curr_idx].run_id == g_search_id && g_nodes[curr_idx].closed) {
            continue;
        }
        g_nodes[curr_idx].closed = 1;

        int cx = curr_idx % map_w;
        int cy = curr_idx / map_w;

        if (curr_idx == target_idx) {
            reached = true;
            break;
        }

        uint16_t h_curr = heuristic(cx, cy, target_tx, target_ty);
        if (h_curr < closest_h) {
            closest_h = h_curr;
            closest_idx = curr_idx;
        }

        for (int d = 0; d < 8; d++) {
            int nx = cx + dirs[d][0];
            int ny = cy + dirs[d][1];
            int step_cost = dirs[d][2];

            if (nx < 1 || nx >= (int)map_w - 1 || ny < 1 || ny >= (int)map->height - 1) {
                continue;
            }

            // Diagonal corner check: don't cut corners of solid tiles
            if (d >= 4) {
                if (!pathfind_is_tile_walkable(map, tileset, houses, cx + dirs[d][0], cy) ||
                    !pathfind_is_tile_walkable(map, tileset, houses, cx, cy + dirs[d][1])) {
                    continue;
                }
            }

            if (!pathfind_is_tile_walkable(map, tileset, houses, nx, ny)) {
                continue;
            }

            uint16_t n_idx = (uint16_t)(ny * map_w + nx);
            if (g_nodes[n_idx].run_id == g_search_id && g_nodes[n_idx].closed) {
                continue;
            }

            uint16_t tent_g = g_nodes[curr_idx].g_cost + (uint16_t)step_cost;
            bool is_new = (g_nodes[n_idx].run_id != g_search_id);

            if (is_new || tent_g < g_nodes[n_idx].g_cost) {
                g_nodes[n_idx].run_id = g_search_id;
                g_nodes[n_idx].parent = curr_idx;
                g_nodes[n_idx].g_cost = tent_g;
                g_nodes[n_idx].f_cost = tent_g + heuristic(nx, ny, target_tx, target_ty);
                g_nodes[n_idx].closed = 0;
                heap_push(n_idx);
            }
        }
    }

    uint16_t end_idx = reached ? target_idx : closest_idx;
    if (end_idx == start_idx) {
        return false;
    }

    // Reconstruct raw tile path
    static uint16_t raw_tiles[MAX_PATH_NODES];
    int raw_count = 0;
    uint16_t backtrack = end_idx;
    while (backtrack != start_idx && backtrack != 0xFFFF && raw_count < MAX_PATH_NODES) {
        raw_tiles[raw_count++] = backtrack;
        backtrack = g_nodes[backtrack].parent;
    }

    if (raw_count == 0) return false;

    // Convert to un-smoothed world points
    static path_point_t un_smoothed[MAX_PATH_NODES + 1];
    int u_count = 0;
    un_smoothed[u_count++] = (path_point_t){ start_x, start_y };
    for (int i = raw_count - 1; i >= 0; i--) {
        uint16_t t_idx = raw_tiles[i];
        int tx = t_idx % map_w;
        int ty = t_idx / map_w;
        un_smoothed[u_count++] = (path_point_t){ tx * 16.0f + 8.0f, ty * 16.0f + 8.0f };
    }
    if (reached && target_was_walkable) {
        un_smoothed[u_count - 1] = (path_point_t){ target_x, target_y };
    }

    // Line-of-sight raycast string-pulling (smoothing)
    out_path->count = 0;
    int curr_p = 0;
    while (curr_p < u_count && out_path->count < MAX_PATH_NODES) {
        // Look ahead as far as possible with unobstructed line of sight
        int furthest = curr_p + 1;
        for (int k = u_count - 1; k > curr_p + 1; k--) {
            if (is_line_walkable(map, tileset, houses, un_smoothed[curr_p].x, un_smoothed[curr_p].y,
                                 un_smoothed[k].x, un_smoothed[k].y)) {
                furthest = k;
                break;
            }
        }
        if (furthest < u_count) {
            out_path->points[out_path->count++] = un_smoothed[furthest];
            curr_p = furthest;
        } else {
            break;
        }
    }

    return (out_path->count > 0);
}
