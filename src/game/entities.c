#include "entities.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

void entity_manager_init(entity_manager_t *mgr) {
    if (!mgr) return;
    memset(mgr, 0, sizeof(entity_manager_t));
}

baldie_t* entity_spawn(entity_manager_t *mgr, baldie_team_t team, baldie_role_t role, float x, float y) {
    if (!mgr) return NULL;

    for (int i = 0; i < MAX_BALDIES; i++) {
        if (!mgr->units[i].active) {
            baldie_t *b = &mgr->units[i];
            b->active = true;
            b->team = team;
            b->role = role;
            b->state = STATE_IDLE;
            b->x = x;
            b->y = y;
            b->target_x = x;
            b->target_y = y;
            b->house_id = -1;
            b->health = 100;
            b->anim_frame = 0;
            b->anim_timer = 0;
            mgr->count++;
            return b;
        }
    }
    return NULL;
}

void entity_update_all(entity_manager_t *mgr) {
    if (!mgr) return;

    for (int i = 0; i < MAX_BALDIES; i++) {
        baldie_t *b = &mgr->units[i];
        if (!b->active) continue;

        if (b->state == STATE_INSIDE_HOUSE) {
            continue; // Handled by house simulation
        }

        // Move towards target
        float dx = b->target_x - b->x;
        float dy = b->target_y - b->y;
        float dist = sqrtf(dx * dx + dy * dy);

        if (dist > 2.0f) {
            b->state = STATE_WALKING;
            float speed = 2.0f;
            b->x += (dx / dist) * speed;
            b->y += (dy / dist) * speed;

            b->anim_timer++;
            if (b->anim_timer >= 4) {
                b->anim_timer = 0;
                b->anim_frame = (b->anim_frame + 1) % 4;
            }
        } else {
            b->state = STATE_IDLE;
            // Idle wander AI: occasionally pick a nearby point
            if ((rand() % 120) == 0) {
                float ox = (float)((rand() % 96) - 48);
                float oy = (float)((rand() % 96) - 48);
                b->target_x = b->x + ox;
                b->target_y = b->y + oy;
            }
        }
    }
}

void entity_render_all(const entity_manager_t *mgr, surface_t *dest, int camera_x, int camera_y, const bal_palette_t *palette) {
    if (!mgr || !dest) return;
    (void)palette;

    for (int i = 0; i < MAX_BALDIES; i++) {
        const baldie_t *b = &mgr->units[i];
        if (!b->active || b->state == STATE_INSIDE_HOUSE) continue;

        int sx = (int)b->x - camera_x;
        int sy = (int)b->y - camera_y;

        // Viewport bounds check
        if (sx < -20 || sx > (int)dest->width || sy < -20 || sy > (int)dest->height) {
            continue;
        }

        // Color based on role & team
        uint32_t body_color = 0xFFFF2222; // Red (Worker)
        if (b->team == TEAM_PLAYER) {
            switch (b->role) {
                case ROLE_WORKER:    body_color = 0xFFFF3333; break; // Red
                case ROLE_BUILDER:   body_color = 0xFF3388FF; break; // Blue
                case ROLE_SCIENTIST: body_color = 0xFFFFFFFF; break; // White
                case ROLE_SOLDIER:   body_color = 0xFF33DD33; break; // Green
            }
        } else {
            body_color = 0xFF884422; // Enemy Hairies (Brown/Dark)
        }

        uint32_t head_color = (b->team == TEAM_PLAYER) ? 0xFFFFCC99 : 0xFF553311; // Skin or Hairy
        uint32_t shadow_color = 0x66000000;

        // Shadow ellipse
        surface_fill_rect(dest, sx + 2, sy + 18, 12, 4, shadow_color);

        // Body / Overalls
        int bounce = (b->state == STATE_WALKING && (b->anim_frame % 2 == 1)) ? 1 : 0;
        surface_fill_rect(dest, sx + 3, sy + 8 - bounce, 10, 10, body_color);
        surface_draw_rect(dest, sx + 3, sy + 8 - bounce, 10, 10, 0xFF000000);

        // Bald Head
        surface_fill_rect(dest, sx + 4, sy - bounce, 8, 8, head_color);
        surface_draw_rect(dest, sx + 4, sy - bounce, 8, 8, 0xFF442200);

        // Eyes
        surface_fill_rect(dest, sx + 6, sy + 3 - bounce, 2, 2, 0xFF000000);
        surface_fill_rect(dest, sx + 9, sy + 3 - bounce, 2, 2, 0xFF000000);

        // Feet (walking leg alternation)
        int leg_off = (b->anim_frame == 1) ? 2 : ((b->anim_frame == 3) ? -2 : 0);
        surface_fill_rect(dest, sx + 3, sy + 18 - bounce, 4, 3 + leg_off, 0xFF222222);
        surface_fill_rect(dest, sx + 9, sy + 18 - bounce, 4, 3 - leg_off, 0xFF222222);
    }
}
