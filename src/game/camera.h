#ifndef CAMERA_H
#define CAMERA_H

#include <stdint.h>
#include "platform/platform.h"

typedef struct {
    int x;
    int y;
    int max_x;
    int max_y;
    int speed;
} camera_t;

void camera_init(camera_t *cam, int start_x, int start_y, int map_w_tiles, int map_h_tiles, int vp_w, int vp_h);
void camera_update(camera_t *cam, const platform_input_t *input, int vp_w, int vp_h);
void camera_clamp(camera_t *cam);

#endif // CAMERA_H
