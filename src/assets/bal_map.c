#include "bal_map.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static FILE* open_asset_file(const char *subpath) {
    char path_buf[256];
    const char *prefixes[] = { "", "../", "../../", "I:/Baldies/", NULL };

    for (int i = 0; prefixes[i] != NULL; i++) {
        snprintf(path_buf, sizeof(path_buf), "%s%s", prefixes[i], subpath);
        FILE *f = fopen(path_buf, "rb");
        if (f) return f;
    }
    return NULL;
}

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
    out_map->width = (uint16_t)((lev_buf[0x22] << 8) | lev_buf[0x23]);
    out_map->height = (uint16_t)((lev_buf[0x24] << 8) | lev_buf[0x25]);

    // Initial camera at offset 0x44 (Little-Endian uint16 cam_x, uint16 cam_y)
    out_map->start_cam_x = (uint16_t)(lev_buf[0x44] | (lev_buf[0x45] << 8));
    out_map->start_cam_y = (uint16_t)(lev_buf[0x46] | (lev_buf[0x47] << 8));

    // Fallbacks if zero or uninitialized
    if (out_map->width == 0) out_map->width = 116;
    if (out_map->height == 0) out_map->height = 112;
    if (out_map->start_cam_x == 0) out_map->start_cam_x = 880;
    if (out_map->start_cam_y == 0) out_map->start_cam_y = 688;

    out_map->total_tiles = (uint32_t)out_map->width * out_map->height;

    // 2. Load MAP file
    char map_filename[64];
    snprintf(map_filename, sizeof(map_filename), "LVL/%s.BAL", out_map->map_file);
    FILE *f_map = open_asset_file(map_filename);
    if (!f_map) {
        snprintf(map_filename, sizeof(map_filename), "lvl/%s.bal", out_map->map_file);
        f_map = open_asset_file(map_filename);
        if (!f_map) return false;
    }

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
