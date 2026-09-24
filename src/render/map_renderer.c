#include "map_renderer.h"

void map_renderer_draw(surface_t *dest,
                       const bal_map_t *map,
                       const bal_tileset_t *tileset,
                       const bal_palette_t *palette,
                       int camera_x, int camera_y) {
    if (!dest || !map || !tileset || !palette) return;

    int dw = (int)dest->width;
    int dh = (int)dest->height;

    // Calculate visible tile ranges
    int start_tx = camera_x / 16;
    int start_ty = camera_y / 16;
    if (camera_x < 0) start_tx = (camera_x - 15) / 16;
    if (camera_y < 0) start_ty = (camera_y - 15) / 16;

    int end_tx = (camera_x + dw + 15) / 16;
    int end_ty = (camera_y + dh + 15) / 16;

    // Clamp to map tile grid
    if (start_tx < 0) start_tx = 0;
    if (start_ty < 0) start_ty = 0;
    if (end_tx > (int)map->width) end_tx = (int)map->width;
    if (end_ty > (int)map->height) end_ty = (int)map->height;

    for (int ty = start_ty; ty < end_ty; ty++) {
        int screen_y = (ty * 16) - camera_y;
        for (int tx = start_tx; tx < end_tx; tx++) {
            int screen_x = (tx * 16) - camera_x;
            uint16_t tile_idx = bal_map_get_tile(map, (uint32_t)tx, (uint32_t)ty);
            const uint8_t *tile_pixels = bal_tileset_get_tile(tileset, (uint32_t)tile_idx);

            if (tile_pixels) {
                surface_blit_tile(dest, tile_pixels, screen_x, screen_y, palette);
            }
        }
    }
}

void map_renderer_draw_minimap(surface_t *dest,
                              const bal_map_t *map,
                              const bal_tileset_t *tileset,
                              const bal_palette_t *palette,
                              int dest_x, int dest_y,
                              int camera_x, int camera_y,
                              int vp_w, int vp_h) {
    if (!dest || !map || !tileset || !palette) return;

    int mw = (int)map->width;
    int mh = (int)map->height;

    // Draw background border
    surface_fill_rect(dest, dest_x - 2, dest_y - 2, mw + 4, mh + 4, 0xFF333333);
    surface_draw_rect(dest, dest_x - 2, dest_y - 2, mw + 4, mh + 4, 0xFFCCCCCC);

    // Draw 1 pixel per tile
    for (int y = 0; y < mh; y++) {
        for (int x = 0; x < mw; x++) {
            uint16_t tile_idx = bal_map_get_tile(map, (uint32_t)x, (uint32_t)y);
            const uint8_t *tile_data = bal_tileset_get_tile(tileset, (uint32_t)tile_idx);
            uint32_t col = 0xFF000000;
            if (tile_data) {
                // Sample center pixel (8, 8)
                uint8_t c = tile_data[8 * 16 + 8];
                col = palette->bgra[c];
            }
            int px = dest_x + x;
            int py = dest_y + y;
            if (px >= 0 && px < (int)dest->width && py >= 0 && py < (int)dest->height) {
                dest->pixels[py * dest->width + px] = col;
            }
        }
    }

    // Draw camera rectangle on mini-map
    int cam_mx = dest_x + (camera_x / 16);
    int cam_my = dest_y + (camera_y / 16);
    int cam_mw = (vp_w + 15) / 16;
    int cam_mh = (vp_h + 15) / 16;
    surface_draw_rect(dest, cam_mx, cam_my, cam_mw, cam_mh, 0xFF00FFFF); // Cyan camera box
}

