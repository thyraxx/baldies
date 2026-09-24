#include "camera.h"

void camera_init(camera_t *cam, int start_x, int start_y, int map_w_tiles, int map_h_tiles, int vp_w, int vp_h) {
    if (!cam) return;
    cam->x = start_x;
    cam->y = start_y;
    cam->speed = 12; // Pixels per frame

    int total_map_w = map_w_tiles * 16;
    int total_map_h = map_h_tiles * 16;


    cam->max_x = (total_map_w > vp_w) ? (total_map_w - vp_w) : 0;
    cam->max_y = (total_map_h > vp_h) ? (total_map_h - vp_h) : 0;
    camera_clamp(cam);
}

void camera_update(camera_t *cam, const platform_input_t *input, int vp_w, int vp_h) {
    if (!cam || !input) return;

    // Keyboard panning
    if (input->key_left)  cam->x -= cam->speed;
    if (input->key_right) cam->x += cam->speed;
    if (input->key_up)    cam->y -= cam->speed;
    if (input->key_down)  cam->y += cam->speed;

    // Mouse edge scrolling (within 16 pixels of screen boundaries)
    int edge_margin = 16;
    if (input->mouse_x >= 0 && input->mouse_x < edge_margin) {
        cam->x -= cam->speed;
    } else if (input->mouse_x > (vp_w - edge_margin) && input->mouse_x <= vp_w) {
        cam->x += cam->speed;
    }

    if (input->mouse_y >= 0 && input->mouse_y < edge_margin) {
        cam->y -= cam->speed;
    } else if (input->mouse_y > (vp_h - edge_margin) && input->mouse_y <= vp_h) {
        cam->y += cam->speed;
    }

    camera_clamp(cam);
}

void camera_clamp(camera_t *cam) {
    if (!cam) return;
    if (cam->x < 0) cam->x = 0;
    if (cam->y < 0) cam->y = 0;
    if (cam->x > cam->max_x) cam->x = cam->max_x;
    if (cam->y > cam->max_y) cam->y = cam->max_y;
}
