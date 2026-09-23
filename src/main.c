#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include "platform/platform.h"
#include "render/surface.h"
#include "game/game_loop.h"
#include "assets/asset_path.h"

int main(int argc, char **argv) {
    asset_system_init();

    int win_w = 1280;
    int win_h = 960;
    bool fullscreen = false;
    uint32_t start_level = 1;

    if (argc >= 3) {
        win_w = atoi(argv[1]);
        win_h = atoi(argv[2]);
        if (win_w < 640) win_w = 640;
        if (win_h < 480) win_h = 480;
    }
    if (argc >= 4) {
        start_level = (uint32_t)atoi(argv[3]);
        if (start_level < 1) start_level = 1;
    }

    printf("======================================================\n");
    printf("            BALDIES (MODERN C ENGINE)                 \n");
    printf("======================================================\n");
    printf("Resolution: %d x %d | Level: %u\n", win_w, win_h, start_level);
    printf("Controls:\n");
    printf("  Arrow Keys / WASD : Pan camera\n");
    printf("  Mouse Screen Edge : Edge scroll\n");
    printf("  Left Click Unit   : Select / change role / issue orders\n");
    printf("  Keys 1 - 4        : Select role (Red/Blue/White/Green)\n");
    printf("  Key M             : Toggle mini-map\n");
    printf("  Alt + Enter       : Toggle fullscreen\n");
    printf("  Escape            : Exit game\n");
    printf("======================================================\n\n");

    if (!platform_init("Baldies (C Engine)", win_w, win_h, fullscreen)) {
        fprintf(stderr, "[ERROR] Failed to initialize platform display window!\n");
        return 1;
    }

    surface_t *render_surface = surface_create((uint32_t)win_w, (uint32_t)win_h);
    if (!render_surface) {
        fprintf(stderr, "[ERROR] Failed to allocate render surface!\n");
        platform_shutdown();
        return 1;
    }

    game_state_t game;
    if (!game_init(&game, start_level, win_w, win_h)) {
        fprintf(stderr, "[ERROR] Failed to initialize game state!\n");
        surface_destroy(render_surface);
        platform_shutdown();
        return 1;
    }

    // Main Loop Timing: 30 TPS Simulation, 60 FPS Presentation
    const uint64_t tick_interval_ms = 33; // ~30.3 ticks/sec
    uint64_t last_tick_time = platform_get_time_ms();

    platform_input_t input = {0};

    while (!input.quit_requested && !input.key_escape) {
        uint64_t frame_start = platform_get_time_ms();

        // 1. Poll input events
        platform_poll_events(&input);

        // Check if window resized
        int cur_w, cur_h;
        platform_get_window_size(&cur_w, &cur_h);
        if (cur_w > 0 && cur_h > 0 && (cur_w != (int)render_surface->width || cur_h != (int)render_surface->height)) {
            surface_destroy(render_surface);
            render_surface = surface_create((uint32_t)cur_w, (uint32_t)cur_h);
            game.camera.max_x = ((int)game.map.width * 32 > cur_w) ? ((int)game.map.width * 32 - cur_w) : 0;
            game.camera.max_y = ((int)game.map.height * 32 > (cur_h - 32)) ? ((int)game.map.height * 32 - (cur_h - 32)) : 0;
        }

        // 2. Fixed-rate 30 TPS simulation updates
        uint64_t now = platform_get_time_ms();
        while (now - last_tick_time >= tick_interval_ms) {
            game_tick(&game, &input, render_surface->width, render_surface->height);
            last_tick_time += tick_interval_ms;
        }

        // 3. Render 60 FPS frame
        surface_clear(render_surface, 0xFF000000);
        game_render(&game, render_surface);

        // 4. Present to screen
        platform_present(render_surface);

        // 5. Frame limiter (~60 FPS / 16ms)
        uint64_t elapsed = platform_get_time_ms() - frame_start;
        if (elapsed < 16) {
            platform_sleep_ms((uint32_t)(16 - elapsed));
        }
    }

    printf("Shutting down Baldies C Engine...\n");
    game_shutdown(&game);
    surface_destroy(render_surface);
    platform_shutdown();

    printf("Exited cleanly.\n");
    return 0;
}
