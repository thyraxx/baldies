#ifndef HOUSE_UI_H
#define HOUSE_UI_H

#include <stdint.h>
#include <stdbool.h>
#include "platform/platform.h"
#include "render/surface.h"
#include "assets/bal_palette.h"
#include "assets/bal_sprites.h"
#include "entities.h"
#include "house.h"

// Forward declaration of game_state_t to avoid circular dependencies
typedef struct game_state_s game_state_t;

typedef struct {
    bool is_open;
    int house_id;          // Index of open house in house_mgr (-1 if none)
    int hovered_elem;      // 0..3: rooms, 4: close button, -1: none
    int hold_role;         // Role currently being held down (-1: none)
    int hold_ticks;        // Number of ticks mouse has been held down on room
    int next_eject_delay;  // Cooldown ticks before next ejection
    int anim_tick;

    // Window layout cached coordinates
    int win_x;
    int win_y;
    int win_w;
    int win_h;
} house_ui_t;

void house_ui_init(house_ui_t *ui);
void house_ui_open(house_ui_t *ui, int house_id);
void house_ui_close(house_ui_t *ui);

// Update and input handler. Returns true if mouse/keyboard was consumed by the House UI.
bool house_ui_update(house_ui_t *ui, game_state_t *game, const platform_input_t *input, int vp_w, int vp_h);

// Render the House UI window
void house_ui_render(const house_ui_t *ui, const game_state_t *game, surface_t *dest);

// Eject 1 unit of role out the front door
bool house_ui_eject_unit(house_ui_t *ui, game_state_t *game, int role);

#endif // HOUSE_UI_H
