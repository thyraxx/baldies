#include "house.h"
#include <string.h>

void house_manager_init(house_manager_t *mgr) {
    if (!mgr) return;
    memset(mgr, 0, sizeof(house_manager_t));
}

house_t* house_create(house_manager_t *mgr, baldie_team_t team, house_tier_t tier, int world_x, int world_y) {
    if (!mgr) return NULL;

    for (int i = 0; i < MAX_HOUSES; i++) {
        if (!mgr->houses[i].active) {
            house_t *h = &mgr->houses[i];
            h->active = true;
            h->team = team;
            h->tier = tier;
            h->world_x = world_x;
            h->world_y = world_y;
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
                // Birth a new Baldie out the front door!
                float spawn_x = (float)(h->world_x + 16);
                float spawn_y = (float)(h->world_y + 48);
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

        int sx = h->world_x - camera_x;
        int sy = h->world_y - camera_y;

        int house_w = 48;
        int house_h = 48;

        if (sx < -house_w || sx > (int)dest->width || sy < -house_h || sy > (int)dest->height) {
            continue;
        }

        // Room occupancy flags floating above the cottage roof (only when units are inside)
        int total_inside = h->rooms[0] + h->rooms[1] + h->rooms[2] + h->rooms[3];
        if (total_inside > 0) {
            // Semi-transparent badge background
            surface_fill_rect(dest, sx + 4, sy - 8, 40, 9, 0xCC111111);
            surface_draw_rect(dest, sx + 4, sy - 8, 40, 9, 0xFFDAA520);

            // 4 colored indicator squares: Red (Workers), Blue (Builders), White (Scientists), Green (Soldiers)
            if (h->rooms[0] > 0) surface_fill_rect(dest, sx + 7,  sy - 6, 6, 5, 0xFFFF2222);
            if (h->rooms[1] > 0) surface_fill_rect(dest, sx + 16, sy - 6, 6, 5, 0xFF3388FF);
            if (h->rooms[2] > 0) surface_fill_rect(dest, sx + 25, sy - 6, 6, 5, 0xFFFFFFFF);
            if (h->rooms[3] > 0) surface_fill_rect(dest, sx + 34, sy - 6, 6, 5, 0xFF33DD33);
        }
    }
}

