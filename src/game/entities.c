#include "entities.h"
#include "house.h"
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
            b->waypoint_count = 0;
            b->waypoint_index = 0;
            mgr->count++;
            return b;
        }
    }
    return NULL;
}

void entity_set_path(baldie_t *b, const float *pts_x, const float *pts_y, int count) {
    if (!b || !pts_x || !pts_y || count <= 0) return;
    if (count > MAX_WAYPOINTS) count = MAX_WAYPOINTS;
    for (int i = 0; i < count; i++) {
        b->waypoints_x[i] = pts_x[i];
        b->waypoints_y[i] = pts_y[i];
    }
    b->waypoint_count = count;
    b->waypoint_index = 0;
    b->state = STATE_WALKING;
    b->target_x = pts_x[count - 1] - 8.0f;
    b->target_y = pts_y[count - 1] - 12.0f;
}

static bool is_baldie_position_walkable(const bal_map_t *map, const bal_tileset_t *tileset, const house_manager_t *houses, float bx, float by) {
    if (!map) return true;
    // Check feet (left, center, right)
    if (!bal_map_is_walkable(map, tileset, bx + 5.0f, by + 13.0f)) return false;
    if (!bal_map_is_walkable(map, tileset, bx + 8.0f, by + 13.0f)) return false;
    if (!bal_map_is_walkable(map, tileset, bx + 11.0f, by + 13.0f)) return false;
    // Check body center
    if (!bal_map_is_walkable(map, tileset, bx + 8.0f, by + 8.0f)) return false;

    // Check dynamic house solid walls/roof
    if (houses) {
        if (house_manager_is_point_blocked(houses, bx + 5.0f, by + 13.0f)) return false;
        if (house_manager_is_point_blocked(houses, bx + 8.0f, by + 13.0f)) return false;
        if (house_manager_is_point_blocked(houses, bx + 11.0f, by + 13.0f)) return false;
        if (house_manager_is_point_blocked(houses, bx + 8.0f, by + 8.0f)) return false;
    }
    return true;
}

void entity_update_all(entity_manager_t *mgr, const bal_map_t *map, const bal_tileset_t *tileset, const house_manager_t *houses) {
    if (!mgr) return;

    for (int i = 0; i < MAX_BALDIES; i++) {
        baldie_t *b = &mgr->units[i];
        if (!b->active) continue;

        if (b->state == STATE_INSIDE_HOUSE) {
            continue; // Handled by house simulation
        }

        // 1. Waypoint-based movement (from pathfinding)
        if (b->waypoint_count > 0 && b->waypoint_index < b->waypoint_count) {
            float cur_wx = b->waypoints_x[b->waypoint_index];
            float cur_wy = b->waypoints_y[b->waypoint_index];

            float feet_x = b->x + 8.0f;
            float feet_y = b->y + 12.0f;
            float dx = cur_wx - feet_x;
            float dy = cur_wy - feet_y;
            float dist = sqrtf(dx * dx + dy * dy);

            if (dist < 4.0f) {
                // Reached this waypoint! Advance to next
                b->waypoint_index++;
                if (b->waypoint_index >= b->waypoint_count) {
                    b->waypoint_count = 0;
                    b->waypoint_index = 0;
                    b->state = STATE_IDLE;
                }
            } else {
                b->state = STATE_WALKING;
                float speed = 1.5f;
                if (dist < speed) speed = dist;
                float step_x = (dx / dist) * speed;
                float step_y = (dy / dist) * speed;

                float next_x = b->x + step_x;
                float next_y = b->y + step_y;

                if (is_baldie_position_walkable(map, tileset, houses, next_x, next_y)) {
                    b->x = next_x;
                    b->y = next_y;
                } else if (is_baldie_position_walkable(map, tileset, houses, next_x, b->y)) {
                    b->x = next_x;
                } else if (is_baldie_position_walkable(map, tileset, houses, b->x, next_y)) {
                    b->y = next_y;
                } else {
                    // Try to advance waypoint if blocked
                    b->waypoint_index++;
                    if (b->waypoint_index >= b->waypoint_count) {
                        b->waypoint_count = 0;
                        b->waypoint_index = 0;
                        b->state = STATE_IDLE;
                    }
                }

                b->anim_timer++;
                if (b->anim_timer >= 4) {
                    b->anim_timer = 0;
                    b->anim_frame = (b->anim_frame + 1) % 4;
                }
            }
        } else {
            // 2. Direct movement fallback (or idle wander)
            float dx = b->target_x - b->x;
            float dy = b->target_y - b->y;
            float dist = sqrtf(dx * dx + dy * dy);

            if (dist > 2.0f) {
                b->state = STATE_WALKING;
                float speed = 1.5f;
                float step_x = (dx / dist) * speed;
                float step_y = (dy / dist) * speed;

                float next_x = b->x + step_x;
                float next_y = b->y + step_y;

                if (is_baldie_position_walkable(map, tileset, houses, next_x, next_y)) {
                    b->x = next_x;
                    b->y = next_y;
                } else if (is_baldie_position_walkable(map, tileset, houses, next_x, b->y)) {
                    b->x = next_x;
                } else if (is_baldie_position_walkable(map, tileset, houses, b->x, next_y)) {
                    b->y = next_y;
                } else {
                    b->target_x = b->x;
                    b->target_y = b->y;
                    b->state = STATE_IDLE;
                }

                b->anim_timer++;
                if (b->anim_timer >= 4) {
                    b->anim_timer = 0;
                    b->anim_frame = (b->anim_frame + 1) % 4;
                }
            } else {
                b->state = STATE_IDLE;
                // Idle wander AI: occasionally pick a nearby walkable point on land
                if ((rand() % 120) == 0) {
                    float ox = (float)((rand() % 32) - 16);
                    float oy = (float)((rand() % 32) - 16);
                    float cand_x = b->x + ox;
                    float cand_y = b->y + oy;
                    if (is_baldie_position_walkable(map, tileset, houses, cand_x, cand_y)) {
                        b->target_x = cand_x;
                        b->target_y = cand_y;
                    }
                }
            }
        }
    }
}



void entity_render_all(const entity_manager_t *mgr, surface_t *dest, int camera_x, int camera_y, const bal_palette_t *palette, const bal_sprites_t *sprites) {
    if (!mgr || !dest) return;

    for (int i = 0; i < MAX_BALDIES; i++) {
        const baldie_t *b = &mgr->units[i];
        if (!b->active || b->state == STATE_INSIDE_HOUSE) continue;

        int sx = (int)b->x - camera_x;
        int sy = (int)b->y - camera_y;

        // Viewport bounds check (16x16 sprite)
        if (sx < -16 || sx > (int)dest->width || sy < -16 || sy > (int)dest->height) {
            continue;
        }

        // Draw shadow under unit
        surface_fill_rect(dest, sx + 2, sy + 13, 12, 3, 0x66000000);

        if (sprites && sprites->player_data) {
            // Authentic 1995 animated Baldie / Hairy sprite!
            bal_sprites_draw_baldie(dest, sprites, (b->team == TEAM_ENEMY), (int)b->role, b->anim_frame, sx, sy, palette);
        } else {
            // Procedural fallback
            uint32_t body_color = (b->role == ROLE_BUILDER) ? 0xFF3388FF :
                                 ((b->role == ROLE_SCIENTIST) ? 0xFFFFFFFF :
                                 ((b->role == ROLE_SOLDIER) ? 0xFF33DD33 : 0xFFFF3333));
            if (b->team == TEAM_ENEMY) body_color = 0xFF884422;
            surface_fill_rect(dest, sx + 3, sy + 6, 10, 8, body_color);
            surface_fill_rect(dest, sx + 4, sy, 8, 6, 0xFFFFCC99);
        }
    }
}
