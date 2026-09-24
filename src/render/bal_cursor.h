#ifndef BAL_CURSOR_H
#define BAL_CURSOR_H

#include <stdint.h>
#include <stdbool.h>
#include "surface.h"
#include "assets/bal_palette.h"

#define CURSOR_FRAME_POINTER      0
#define CURSOR_FRAME_AREA_SELECT  1
#define CURSOR_FRAME_OPEN_HAND    4
#define CURSOR_FRAME_GRAB_HAND    5
#define CURSOR_FRAME_ENEMY_OPEN   11
#define CURSOR_FRAME_ENEMY_GRAB   12
#define CURSOR_FRAME_HOURGLASS    13

typedef struct {
    uint8_t *data;          // 18 frames of 32x32 = 18432 bytes
    uint32_t num_frames;
} bal_cursor_t;

bool bal_cursor_init(bal_cursor_t *cur);
void bal_cursor_free(bal_cursor_t *cur);
void bal_cursor_draw(surface_t *dest, const bal_cursor_t *cur, int frame_idx, int x, int y, const bal_palette_t *palette);

#endif // BAL_CURSOR_H
