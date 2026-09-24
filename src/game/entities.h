#ifndef ENTITIES_H
#define ENTITIES_H

#include <stdint.h>
#include <stdbool.h>
#include "render/surface.h"
#include "assets/bal_palette.h"

#define MAX_BALDIES 256

typedef enum {
    ROLE_WORKER = 0,    // Red: Breeding inside houses
    ROLE_BUILDER = 1,   // Blue: Building houses, chopping wood
    ROLE_SCIENTIST = 2, // White: Inventing in laboratory
    ROLE_SOLDIER = 3    // Green: Combat, munitions
} baldie_role_t;

typedef enum {
    TEAM_PLAYER = 0,
    TEAM_ENEMY = 1
} baldie_team_t;

typedef enum {
    STATE_IDLE = 0,
    STATE_WALKING = 1,
    STATE_INSIDE_HOUSE = 2,
    STATE_CHOPPING = 3,
    STATE_FIGHTING = 4,
    STATE_CARRIED = 5
} baldie_state_t;

#define MAX_WAYPOINTS 64

typedef struct {
    bool active;
    baldie_team_t team;
    baldie_role_t role;
    baldie_state_t state;
    float x;
    float y;
    float target_x;
    float target_y;
    int house_id; // -1 if outdoors
    int health;
    int anim_frame;
    uint32_t anim_timer;

    // Pathfinding waypoints
    float waypoints_x[MAX_WAYPOINTS];
    float waypoints_y[MAX_WAYPOINTS];
    int waypoint_count;
    int waypoint_index;
} baldie_t;

typedef struct {
    baldie_t units[MAX_BALDIES];
    uint32_t count;
} entity_manager_t;

#include "assets/bal_sprites.h"
#include "assets/bal_map.h"

struct house_manager_s;
typedef struct house_manager_s house_manager_t;

void entity_manager_init(entity_manager_t *mgr);
baldie_t* entity_spawn(entity_manager_t *mgr, baldie_team_t team, baldie_role_t role, float x, float y);
void entity_set_path(baldie_t *b, const float *pts_x, const float *pts_y, int count);
void entity_update_all(entity_manager_t *mgr, const bal_map_t *map, const bal_tileset_t *tileset, const house_manager_t *houses);
void entity_render_all(const entity_manager_t *mgr, surface_t *dest, int camera_x, int camera_y, const bal_palette_t *palette, const bal_sprites_t *sprites);
bool entity_is_position_walkable(const bal_map_t *map, const bal_tileset_t *tileset, const house_manager_t *houses, float bx, float by);

#endif // ENTITIES_H

