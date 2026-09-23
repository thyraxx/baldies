#ifndef MAP_RENDERER_H
#define MAP_RENDERER_H

#include "surface.h"
#include "assets/bal_map.h"
#include "assets/bal_tiles.h"
#include "assets/bal_palette.h"

void map_renderer_draw(surface_t *dest,
                       const bal_map_t *map,
                       const bal_tileset_t *tileset,
                       const bal_palette_t *palette,
                       int camera_x, int camera_y);

void map_renderer_draw_minimap(surface_t *dest,
                              const bal_map_t *map,
                              const bal_tileset_t *tileset,
                              const bal_palette_t *palette,
                              int dest_x, int dest_y,
                              int camera_x, int camera_y,
                              int vp_w, int vp_h);

#endif // MAP_RENDERER_H
