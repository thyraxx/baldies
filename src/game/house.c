#include "house.h"
#include <string.h>

void house_manager_init(house_manager_t *mgr) {
    if (!mgr) return;
    memset(mgr, 0, sizeof(house_manager_t));
}

house_t* house_create(house_manager_t *mgr, baldie_team_t team, house_tier_t tier, int tile_x, int tile_y) {
    if (!mgr) return NULL;

    for (int i = 0; i < MAX_HOUSES; i++) {
        if (!mgr->houses[i].active) {
            house_t *h = &mgr->houses[i];
            h->active = true;
            h->team = team;
            h->tier = tier;
            h->tile_x = tile_x;
            h->tile_y = tile_y;
            h->breed_timer = 0;
            memset(h->rooms, 0, sizeof(h->rooms));
            mgr->count++;
            return h;
        }
    }
    return NULL;
}

void house_update_all(house_manager_t *mgr, entity_manager_t *entity_mgr) {
    if (!mgr || !entity_mgr) return;

    for (int i = 0; i < MAX_HOUSES; i++) {
        house_t *h = &mgr->houses[i];
        if (!h->active) continue;

        // Room 0 (Workers / Breeders)
        int workers = h->rooms[0];
        if (workers > 0) {
            h->breed_timer++;
            // 30 TPS: 300 ticks = 10 seconds per birth per worker
            int breed_threshold = (workers >= 2) ? 150 : 300;
            if (h->breed_timer >= breed_threshold) {
                h->breed_timer = 0;
                // Birth a new Baldie!
                float spawn_x = (float)(h->tile_x * 32 + 16);
                float spawn_y = (float)(h->tile_y * 32 + 40);
                entity_spawn(entity_mgr, h->team, ROLE_WORKER, spawn_x, spawn_y);
            }
        }
    }
}

void house_render_all(const house_manager_t *mgr, surface_t *dest, int camera_x, int camera_y) {
    if (!mgr || !dest) return;

    for (int i = 0; i < MAX_HOUSES; i++) {
        const house_t *h = &mgr->houses[i];
        if (!h->active) continue;

        int sx = (h->tile_x * 32) - camera_x;
        int sy = (h->tile_y * 32) - camera_y;

        int house_w = 48;
        int house_h = 48;

        if (sx < -house_w || sx > (int)dest->width || sy < -house_h || sy > (int)dest->height) {
            continue;
        }

        // Draw house base
        uint32_t wall_color = (h->team == TEAM_PLAYER) ? 0xFF8B5A2B : 0xFF553311;
        uint32_t roof_color = (h->team == TEAM_PLAYER) ? 0xFFB22222 : 0xFF4A4A4A;

        // Walls
        surface_fill_rect(dest, sx, sy + 16, house_w, house_h - 16, wall_color);
        surface_draw_rect(dest, sx, sy + 16, house_w, house_h - 16, 0xFF000000);

        // Roof
        surface_fill_rect(dest, sx - 4, sy, house_w + 8, 18, roof_color);
        surface_draw_rect(dest, sx - 4, sy, house_w + 8, 18, 0xFF000000);

        // Door
        surface_fill_rect(dest, sx + 18, sy + 28, 12, 20, 0xFF3D2314);
        surface_draw_rect(dest, sx + 18, sy + 28, 12, 20, 0xFF000000);

        // Room occupancy flags (4 small colored squares above door)
        if (h->rooms[0] > 0) surface_fill_rect(dest, sx + 4,  sy + 20, 6, 6, 0xFFFF2222); // Red
        if (h->rooms[1] > 0) surface_fill_rect(dest, sx + 12, sy + 20, 6, 6, 0xFF2266FF); // Blue
        if (h->rooms[2] > 0) surface_fill_rect(dest, sx + 30, sy + 20, 6, 6, 0xFFEEEEEE); // White
        if (h->rooms[3] > 0) surface_fill_rect(dest, sx + 38, sy + 20, 6, 6, 0xFF22CC22); // Green
    }
}
