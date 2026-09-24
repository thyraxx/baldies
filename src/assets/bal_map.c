#include "bal_map.h"
#include "asset_path.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define open_asset_file(p) asset_open_file(p, "rb")


bool bal_map_load(uint32_t level_num, bal_map_t *out_map) {
    if (!out_map) return false;
    memset(out_map, 0, sizeof(bal_map_t));

    // 1. Load LEVEL%03d.BAL
    char lev_filename[64];
    snprintf(lev_filename, sizeof(lev_filename), "LVL/LEVEL%03d.BAL", level_num);
    FILE *f_lev = open_asset_file(lev_filename);
    if (!f_lev) {
        // Try uppercase/alternate
        snprintf(lev_filename, sizeof(lev_filename), "lvl/level%03d.bal", level_num);
        f_lev = open_asset_file(lev_filename);
        if (!f_lev) return false;
    }

    uint8_t lev_buf[1024];
    size_t lev_read = fread(lev_buf, 1, sizeof(lev_buf), f_lev);
    fclose(f_lev);

    if (lev_read < 0x50) {
        return false;
    }

    // Parse level name at 0x00
    strncpy(out_map->name, (const char*)&lev_buf[0x00], 16);
    out_map->name[16] = '\0';

    // Parse block file at 0x11
    strncpy(out_map->block_file, (const char*)&lev_buf[0x11], 16);
    out_map->block_file[16] = '\0';

    // Parse map file at 0x1A
    int mi = 0;
    while (mi < 15 && lev_buf[0x1A + mi] >= 32 && lev_buf[0x1A + mi] <= 126) {
        out_map->map_file[mi] = (char)lev_buf[0x1A + mi];
        mi++;
    }
    out_map->map_file[mi] = '\0';

    // Map dimensions at offset 0x22 (Big-Endian uint16 width, uint16 height)
    // Map dimensions: offset 0x22 has rows (height), offset 0x24 has columns (width)
    uint16_t dim_h = (uint16_t)((lev_buf[0x22] << 8) | lev_buf[0x23]);
    uint16_t dim_w = (uint16_t)((lev_buf[0x24] << 8) | lev_buf[0x25]);

    // Initial camera at offset 0x44 (Little-Endian uint16 cam_x, uint16 cam_y)
    out_map->start_cam_x = (uint16_t)(lev_buf[0x44] | (lev_buf[0x45] << 8));
    out_map->start_cam_y = (uint16_t)(lev_buf[0x46] | (lev_buf[0x47] << 8));

    if (out_map->start_cam_x == 0) out_map->start_cam_x = 880;
    if (out_map->start_cam_y == 0) out_map->start_cam_y = 688;

    // Player base is at start camera location
    out_map->player_base_x = out_map->start_cam_x;
    out_map->player_base_y = out_map->start_cam_y;

    // Parse records (each 32 bytes from 0x44 onwards) to find the enemy base
    out_map->enemy_base_x = out_map->player_base_x;
    out_map->enemy_base_y = out_map->player_base_y + 400; // safe fallback

    uint32_t max_dist = 0;
    for (size_t ro = 0x44; ro + 32 <= lev_read; ro += 32) {
        uint16_t rx = (uint16_t)(lev_buf[ro] | (lev_buf[ro + 1] << 8));
        uint16_t ry = (uint16_t)(lev_buf[ro + 2] | (lev_buf[ro + 3] << 8));
        if (rx > 0 && ry > 0 && rx < (dim_w * 16) && ry < (dim_h * 16)) {
            int dx = (int)rx - (int)out_map->player_base_x;
            int dy = (int)ry - (int)out_map->player_base_y;
            uint32_t dist = (uint32_t)(dx * dx + dy * dy);
            if (dist > max_dist) {
                max_dist = dist;
                out_map->enemy_base_x = rx;
                out_map->enemy_base_y = ry;
            }
        }
    }


    // 2. Load MAP file
    char map_filename[64];
    snprintf(map_filename, sizeof(map_filename), "LVL/%s.BAL", out_map->map_file);
    FILE *f_map = open_asset_file(map_filename);
    if (!f_map) {
        snprintf(map_filename, sizeof(map_filename), "lvl/%s.bal", out_map->map_file);
        f_map = open_asset_file(map_filename);
        if (!f_map) return false;
    }

    fseek(f_map, 0, SEEK_END);
    long map_file_sz = ftell(f_map);
    fseek(f_map, 0, SEEK_SET);

    uint32_t total_words = (uint32_t)(map_file_sz / 2);

    // Reconcile width and height with total map words
    if (dim_w > 0 && (total_words % dim_w) == 0) {
        out_map->width = dim_w;
        out_map->height = (uint16_t)(total_words / dim_w);
    } else if (dim_h > 0 && (total_words % dim_h) == 0) {
        out_map->width = (uint16_t)(total_words / dim_h);
        out_map->height = dim_h;
    } else {
        out_map->width = 112;
        out_map->height = 116;
    }

    out_map->total_tiles = (uint32_t)out_map->width * out_map->height;
    size_t map_bytes = out_map->total_tiles * sizeof(uint16_t);

    out_map->tiles = (uint16_t*)malloc(map_bytes);
    if (!out_map->tiles) {
        fclose(f_map);
        return false;
    }

    size_t map_read = fread(out_map->tiles, sizeof(uint16_t), out_map->total_tiles, f_map);
    fclose(f_map);

    if (map_read != out_map->total_tiles) {
        free(out_map->tiles);
        out_map->tiles = NULL;
        return false;
    }

    // Detect player cottage (tile 374) and enemy hut (tile 1210) from map tiles
    for (uint32_t ty = 0; ty < out_map->height; ty++) {
        for (uint32_t tx = 0; tx < out_map->width; tx++) {
            uint16_t t = bal_map_get_tile(out_map, tx, ty);
            if (t == 374) {
                out_map->player_base_x = (uint16_t)(tx * 16);
                out_map->player_base_y = (uint16_t)(ty * 16);
            } else if (t == 1210) {
                out_map->enemy_base_x = (uint16_t)(tx * 16);
                out_map->enemy_base_y = (uint16_t)(ty * 16);
            }
        }
    }

    return true;
}

void bal_map_free(bal_map_t *map) {
    if (map && map->tiles) {
        free(map->tiles);
        map->tiles = NULL;
        map->total_tiles = 0;
    }
}

uint16_t bal_map_get_tile(const bal_map_t *map, uint32_t tile_x, uint32_t tile_y) {
    if (!map || !map->tiles || tile_x >= map->width || tile_y >= map->height) {
        return 0;
    }
    return map->tiles[tile_y * map->width + tile_x];
}

bool bal_map_is_walkable(const bal_map_t *map, const bal_tileset_t *tileset, float x, float y) {
    if (!map || !map->tiles) return false;

    // Hard boundary margins: keep at least 1 tile inside world
    float max_x = (float)(map->width - 1) * 16.0f;
    float max_y = (float)(map->height - 1) * 16.0f;
    if (x < 16.0f || x >= max_x || y < 16.0f || y >= max_y) {
        return false;
    }

    int tx = (int)(x / 16.0f);
    int ty = (int)(y / 16.0f);
    if (tx < 0 || tx >= (int)map->width || ty < 0 || ty >= (int)map->height) {
        return false;
    }

    uint16_t tile = bal_map_get_tile(map, (uint32_t)tx, (uint32_t)ty);

    // 1. Water animation tiles (340 to 359)
    if (tile >= 340 && tile <= 359) {
        return false;
    }

    // 2. Tileset passability classification (water or solid obstacle)
    if (tileset && tile < 1280) {
        uint8_t pass = tileset->passability[tile];
        if (pass == TILE_PASS_WATER || pass == TILE_PASS_SOLID) {
            return false;
        }
    }


    // 3. Pixel-exact inspection on shoreline tiles
    if (tileset && tileset->data && tile < tileset->num_tiles) {
        const uint8_t *tdata = bal_tileset_get_tile(tileset, (uint32_t)tile);
        if (tdata) {
            int px = ((int)x) % 16;
            int py = ((int)y) % 16;
            if (px < 0) px += 16;
            if (py < 0) py += 16;
            uint8_t c = tdata[py * 16 + px];
            if (c >= 37 && c <= 44) {
                return false;
            }
        }
    }

    return true;
}


