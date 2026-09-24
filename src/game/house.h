#ifndef HOUSE_H
#define HOUSE_H

#include <stdint.h>
#include <stdbool.h>
#include "render/surface.h"
#include "entities.h"

#define MAX_HOUSES 32

typedef enum {
    HOUSE_HUT = 0,
    HOUSE_BARRACKS = 1,
    HOUSE_CASTLE = 2
} house_tier_t;

typedef struct {
    bool active;
    baldie_team_t team;
    house_tier_t tier;
    int world_x;
    int world_y;
    int rooms[4]; // Count of Baldies in each room: [0]=Red, [1]=Blue, [2]=White, [3]=Green
    int breed_timer;
} house_t;

typedef struct house_manager_s {
    house_t houses[MAX_HOUSES];
    uint32_t count;
} house_manager_t;

void house_manager_init(house_manager_t *mgr);
house_t* house_create(house_manager_t *mgr, baldie_team_t team, house_tier_t tier, int world_x, int world_y);

bool house_manager_is_point_blocked(const house_manager_t *mgr, float x, float y);
void house_update_all(house_manager_t *mgr, entity_manager_t *entity_mgr);
void house_render_all(const house_manager_t *mgr, surface_t *dest, int camera_x, int camera_y);

#endif // HOUSE_H
