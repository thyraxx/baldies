#ifndef BAL_SPRITES_H
#define BAL_SPRITES_H

#include <stdint.h>
#include <stdbool.h>
#include "render/surface.h"
#include "assets/bal_palette.h"

#define SPRITE_SHEET_W 320
#define SPRITE_SHEET_H 176
#define SPRITE_CELL_W  16
#define SPRITE_CELL_H  16
#define SPRITE_COLS    20
#define SPRITE_ROWS    11
#define TOTAL_SPRITES  (SPRITE_COLS * SPRITE_ROWS) // 220

typedef struct {
    uint8_t *player_data; // 56320 bytes
    uint8_t *enemy_data;  // 56320 bytes
} bal_sprites_t;

bool bal_sprites_init(bal_sprites_t *sprites, uint32_t level_theme);
void bal_sprites_free(bal_sprites_t *sprites);

void bal_sprites_draw_baldie(surface_t *dest,
                             const bal_sprites_t *sprites,
                             bool is_enemy,
                             int role, // 0=Red, 1=Blue, 2=White, 3=Green
                             int anim_frame, // 0..3
                             int dest_x, int dest_y,
                             const bal_palette_t *palette);

#endif // BAL_SPRITES_H
