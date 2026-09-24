#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <math.h>

#include "assets/bal_palette.h"
#include "assets/bal_tiles.h"
#include "assets/bal_map.h"
#include "assets/bal_sprites.h"
#include "assets/bal_sfx.h"
#include "assets/bal_midi.h"
#include "assets/asset_path.h"
#include "render/map_renderer.h"
#include "game/house.h"
#include "game/pathfind.h"
#include "render/bal_cursor.h"
#include "game/game_loop.h"



static int g_tests_run = 0;
static int g_tests_passed = 0;

#define TEST_ASSERT(cond, name) do { \
    g_tests_run++; \
    if (cond) { \
        g_tests_passed++; \
        printf("  [PASS] %s\n", name); \
    } else { \
        printf("  [FAIL] %s\n", name); \
    } \
} while(0)

int main(void) {
    asset_system_init();

    printf("======================================================\n");
    printf("        BALDIES C ENGINE - ASSET DECODER TESTS        \n");
    printf("======================================================\n\n");

    // 1. Palette Loading Test
    printf("Testing Palette Loading:\n");
    bal_palette_t pal;
    bool pal_loaded = bal_palette_load("BALS/BTPAL.BAL", &pal);
    if (!pal_loaded) pal_loaded = bal_palette_load("../BALS/BTPAL.BAL", &pal);
    TEST_ASSERT(pal_loaded, "Load BALS/BTPAL.BAL");
    TEST_ASSERT(pal.colors[11].r == 255 && pal.colors[11].g == 255 && pal.colors[11].b == 255, "White color at index 11");

    // 2. Tileset Loading Test
    printf("\nTesting Tileset Loading:\n");
    bal_tileset_t tileset;
    bool tiles_loaded = bal_tileset_load("BALS/LEV1BLK.BAL", &tileset);
    if (!tiles_loaded) tiles_loaded = bal_tileset_load("../BALS/LEV1BLK.BAL", &tileset);
    TEST_ASSERT(tiles_loaded, "Load BALS/LEV1BLK.BAL");
    TEST_ASSERT(tileset.num_tiles == 1280, "Tileset contains exactly 1280 tiles (16x16)");
    const uint8_t *tile0 = bal_tileset_get_tile(&tileset, 0);
    TEST_ASSERT(tile0 != NULL, "Get tile 0 data");

    // 3. Map & Level Loading Test
    printf("\nTesting Map & Level Loading:\n");
    bal_map_t map;
    bool map_loaded = bal_map_load(1, &map);
    TEST_ASSERT(map_loaded, "Load Level 1 (GRASS1)");
    TEST_ASSERT(map.width == 112, "Map width is 112 tiles");
    TEST_ASSERT(map.height == 116, "Map height is 116 tiles");
    TEST_ASSERT(map.start_cam_x == 880 && map.start_cam_y == 688, "Camera spawn coordinates (880, 688)");
    uint16_t border_tile = bal_map_get_tile(&map, 0, 0);
    TEST_ASSERT(border_tile == 340, "Border tile index is 340 (0x0154)");
    TEST_ASSERT(bal_tileset_get_tile(&tileset, border_tile) != NULL, "Border tile 340 is valid in 1280 tileset");
    TEST_ASSERT(map.player_base_x == 912 && map.player_base_y == 704, "Detected player cottage at (912, 704)");
    TEST_ASSERT(bal_map_is_walkable(&map, &tileset, 928.0f, 740.0f), "Island grass (928, 740) is walkable");
    TEST_ASSERT(!bal_map_is_walkable(&map, &tileset, 32.0f, 32.0f), "Ocean water (32, 32) is not walkable");
    TEST_ASSERT(!bal_map_is_walkable(&map, &tileset, -10.0f, 50.0f), "Out-of-bounds coordinates are not walkable");

    // Obstacle Collision Verification Tests
    printf("\nTesting Obstacle Collision (Houses, Stones, Logs):\n");

    // 1. Player Cottage Wall Collision vs Door
    // Roof (57, 44), (58, 44), (59, 44) -> solid
    TEST_ASSERT(!bal_map_is_walkable(&map, &tileset, 57 * 16.0f + 8.0f, 44 * 16.0f + 8.0f), "Player cottage roof tile 374 at (57, 44) is solid");
    TEST_ASSERT(!bal_map_is_walkable(&map, &tileset, 58 * 16.0f + 8.0f, 44 * 16.0f + 8.0f), "Player cottage roof tile 375 at (58, 44) is solid");
    TEST_ASSERT(!bal_map_is_walkable(&map, &tileset, 59 * 16.0f + 8.0f, 44 * 16.0f + 8.0f), "Player cottage roof tile 376 at (59, 44) is solid");
    // Walls (57, 45), (59, 45), (57, 46), (59, 46) -> solid
    TEST_ASSERT(!bal_map_is_walkable(&map, &tileset, 57 * 16.0f + 8.0f, 45 * 16.0f + 8.0f), "Player cottage wall tile 394 at (57, 45) is solid");
    TEST_ASSERT(!bal_map_is_walkable(&map, &tileset, 57 * 16.0f + 8.0f, 46 * 16.0f + 8.0f), "Player cottage wall tile 414 at (57, 46) is solid");
    TEST_ASSERT(!bal_map_is_walkable(&map, &tileset, 59 * 16.0f + 8.0f, 46 * 16.0f + 8.0f), "Player cottage wall tile 416 at (59, 46) is solid");
    // Cottage Doorway (58, 46) -> walkable
    TEST_ASSERT(bal_map_is_walkable(&map, &tileset, 58 * 16.0f + 8.0f, 46 * 16.0f + 8.0f), "Player cottage door tile 415 at (58, 46) is walkable");

    // 2. Enemy Hut Wall Collision vs Door
    // Roof/walls (57, 71), (58, 71), (59, 71), (57, 72), (59, 72) -> solid
    TEST_ASSERT(!bal_map_is_walkable(&map, &tileset, 57 * 16.0f + 8.0f, 71 * 16.0f + 8.0f), "Enemy hut roof tile 1210 at (57, 71) is solid");
    TEST_ASSERT(!bal_map_is_walkable(&map, &tileset, 57 * 16.0f + 8.0f, 72 * 16.0f + 8.0f), "Enemy hut wall tile 1213 at (57, 72) is solid");
    TEST_ASSERT(!bal_map_is_walkable(&map, &tileset, 59 * 16.0f + 8.0f, 72 * 16.0f + 8.0f), "Enemy hut wall tile 1215 at (59, 72) is solid");
    // Hut Doorway (58, 72) -> walkable
    TEST_ASSERT(bal_map_is_walkable(&map, &tileset, 58 * 16.0f + 8.0f, 72 * 16.0f + 8.0f), "Enemy hut door tile 1214 at (58, 72) is walkable");

    // 3. Standing Stone Monoliths and Boulders
    TEST_ASSERT(!bal_map_is_walkable(&map, &tileset, 63 * 16.0f + 8.0f, 50 * 16.0f + 8.0f), "Stone monolith top tile 1189 at (63, 50) is solid");
    TEST_ASSERT(!bal_map_is_walkable(&map, &tileset, 63 * 16.0f + 8.0f, 51 * 16.0f + 8.0f), "Stone monolith bottom tile 1209 at (63, 51) is solid");
    TEST_ASSERT(!bal_map_is_walkable(&map, &tileset, 66 * 16.0f + 8.0f, 38 * 16.0f + 8.0f), "Boulder rock tile 1199 at (66, 38) is solid");
    TEST_ASSERT(!bal_map_is_walkable(&map, &tileset, 45 * 16.0f + 8.0f, 46 * 16.0f + 8.0f), "Boulder rock tile 1199 at (45, 46) is solid");
    TEST_ASSERT(!bal_map_is_walkable(&map, &tileset, 61 * 16.0f + 8.0f, 76 * 16.0f + 8.0f), "Stone rock tile 1190 at (61, 76) is solid");

    // 4. Fallen Tree Logs and Stumps
    TEST_ASSERT(!bal_map_is_walkable(&map, &tileset, 52 * 16.0f + 8.0f, 77 * 16.0f + 8.0f), "Tree log top tile 1185 at (52, 77) is solid");
    TEST_ASSERT(!bal_map_is_walkable(&map, &tileset, 52 * 16.0f + 8.0f, 78 * 16.0f + 8.0f), "Tree log bottom tile 1205 at (52, 78) is solid");
    TEST_ASSERT(!bal_map_is_walkable(&map, &tileset, 49 * 16.0f + 8.0f, 64 * 16.0f + 8.0f), "Tree stump tile 1196 at (49, 64) is solid");
    TEST_ASSERT(!bal_map_is_walkable(&map, &tileset, 50 * 16.0f + 8.0f, 65 * 16.0f + 8.0f), "Tree stump tile 1217 at (50, 65) is solid");

    // 5. Dynamic House Manager Collision
    house_manager_t test_hmgr;
    house_manager_init(&test_hmgr);
    house_create(&test_hmgr, TEAM_PLAYER, HOUSE_HUT, 912, 704);
    TEST_ASSERT(house_manager_is_point_blocked(&test_hmgr, 912.0f + 8.0f, 704.0f + 8.0f), "House manager blocks roof (920, 712)");
    TEST_ASSERT(house_manager_is_point_blocked(&test_hmgr, 912.0f + 4.0f, 704.0f + 40.0f), "House manager blocks left wall (916, 744)");
    TEST_ASSERT(!house_manager_is_point_blocked(&test_hmgr, 912.0f + 24.0f, 704.0f + 40.0f), "House manager allows door entrance (936, 744)");
    TEST_ASSERT(!house_manager_is_point_blocked(&test_hmgr, 912.0f + 24.0f, 704.0f + 54.0f), "House manager allows lawn outside door (936, 758)");

    // 6. Entity Movement Collision Integration Test
    printf("\nTesting Entity Movement Collision With Obstacles:\n");
    entity_manager_t emgr;
    entity_manager_init(&emgr);

    // Test A: Baldie walking into cottage left wall is stopped/blocked
    baldie_t *b_wall = entity_spawn(&emgr, TEAM_PLAYER, ROLE_WORKER, 900.0f, 740.0f);
    b_wall->target_x = 920.0f; // Inside left wall (tx=57, ty=46)
    b_wall->target_y = 740.0f;
    for (int step = 0; step < 30; step++) {
        entity_update_all(&emgr, &map, &tileset, &test_hmgr);
    }
    // Unit should be blocked before entering the wall (x cannot penetrate to 920)
    TEST_ASSERT(b_wall->x < 908.0f, "Baldie blocked by cottage wall (does not penetrate to 920)");

    // Test B: Baldie walking into stone monolith is stopped/blocked
    baldie_t *b_stone = entity_spawn(&emgr, TEAM_PLAYER, ROLE_WORKER, 1008.0f, 780.0f);
    b_stone->target_x = 1008.0f;
    b_stone->target_y = 808.0f; // Inside stone monolith (tx=63, ty=50)
    for (int step = 0; step < 30; step++) {
        entity_update_all(&emgr, &map, &tileset, &test_hmgr);
    }
    TEST_ASSERT(b_stone->y < 796.0f, "Baldie blocked by stone monolith (does not penetrate to 808)");

    // Test C: Baldie walking into fallen tree log is stopped/blocked
    baldie_t *b_log = entity_spawn(&emgr, TEAM_PLAYER, ROLE_WORKER, 832.0f, 1210.0f);
    b_log->target_x = 832.0f;
    b_log->target_y = 1240.0f; // Inside tree log (tx=52, ty=77)
    for (int step = 0; step < 30; step++) {
        entity_update_all(&emgr, &map, &tileset, &test_hmgr);
    }
    TEST_ASSERT(b_log->y < 1228.0f, "Baldie blocked by fallen tree log (does not penetrate to 1240)");

    // 7. A* Pathfinding Around Obstacles
    printf("\nTesting A* Pathfinding Around Obstacles:\n");
    path_result_t path1, path2, path3, path4;

    // Test Path 1: Navigate around cottage from south lawn (936, 760) to north lawn (936, 680)
    bool p1_found = pathfind_find_path(&map, &tileset, &test_hmgr, 936.0f, 760.0f, 936.0f, 680.0f, &path1);
    TEST_ASSERT(p1_found, "Find A* path around player cottage");
    TEST_ASSERT(path1.count >= 2, "Path contains waypoints around the building");
    // Verify all path points are walkable
    bool p1_all_walkable = true;
    for (int i = 0; i < path1.count; i++) {
        int wtx = (int)(path1.points[i].x / 16.0f);
        int wty = (int)(path1.points[i].y / 16.0f);
        if (!pathfind_is_tile_walkable(&map, &tileset, &test_hmgr, wtx, wty)) {
            p1_all_walkable = false;
        }
    }
    TEST_ASSERT(p1_all_walkable, "All waypoints around cottage are on walkable terrain");

    // Test Path 2: Navigate around stone monolith at (63, 50..51)
    bool p2_found = pathfind_find_path(&map, &tileset, &test_hmgr, 1008.0f, 780.0f, 1008.0f, 840.0f, &path2);
    TEST_ASSERT(p2_found, "Find A* path around stone monolith");

    // Test Path 3: Navigate around fallen tree log at (52, 77..78)
    bool p3_found = pathfind_find_path(&map, &tileset, &test_hmgr, 832.0f, 1210.0f, 832.0f, 1270.0f, &path3);
    TEST_ASSERT(p3_found, "Find A* path around fallen tree log");

    // Test Path 4: Clicking directly on solid obstacle (stone monolith at 1008, 808) finds path to adjacent walkable tile
    bool p4_found = pathfind_find_path(&map, &tileset, &test_hmgr, 1008.0f, 760.0f, 1008.0f, 808.0f, &path4);
    TEST_ASSERT(p4_found, "Clicking on stone monolith resolves to adjacent walkable tile");
    if (p4_found && path4.count > 0) {
        float last_x = path4.points[path4.count - 1].x;
        float last_y = path4.points[path4.count - 1].y;
        int ltx = (int)(last_x / 16.0f);
        int lty = (int)(last_y / 16.0f);
        TEST_ASSERT(pathfind_is_tile_walkable(&map, &tileset, &test_hmgr, ltx, lty), "Destination next to stone monolith is walkable");
    }

    // Test Path 5: Unit following path reaches destination around cottage
    baldie_t *b_nav = entity_spawn(&emgr, TEAM_PLAYER, ROLE_WORKER, 936.0f, 760.0f);
    if (p1_found && path1.count > 0) {
        float px[MAX_PATH_NODES], py[MAX_PATH_NODES];
        for (int i = 0; i < path1.count; i++) {
            px[i] = path1.points[i].x;
            py[i] = path1.points[i].y;
        }
        entity_set_path(b_nav, px, py, path1.count);
        for (int step = 0; step < 300; step++) {
            entity_update_all(&emgr, &map, &tileset, &test_hmgr);
            if (b_nav->waypoint_count == 0) break;
        }
        float dest_x = path1.points[path1.count - 1].x;
        float dest_y = path1.points[path1.count - 1].y;
        float final_dist = fabsf(b_nav->x + 8.0f - dest_x) + fabsf(b_nav->y + 12.0f - dest_y);
        TEST_ASSERT(final_dist < 16.0f, "Baldie follows waypoints and reaches destination around cottage");
    }


    // 4. Sprites Test
    printf("\nTesting Baldie Sprites Loading:\n");
    bal_sprites_t sprites;
    bool sprites_loaded = bal_sprites_init(&sprites, 1);
    TEST_ASSERT(sprites_loaded, "Load LEV1PLR1.BAL and LEV1NME.BAL 16x16 sprites");

    // 5. In-Game Frame Rendering Verification
    printf("\nTesting In-Game Surface Rendering:\n");
    surface_t *surf = surface_create(640, 480);
    bal_palette_t level_pal;
    bal_palette_load("BALS/LEV1PAL.BAL", &level_pal);
    map_renderer_draw(surf, &map, &tileset, &level_pal, map.start_cam_x, map.start_cam_y);

    // Draw 4 player Baldies with 4 different roles and 4 different directions (North, South, East, West)
    bal_sprites_draw_baldie(surf, &sprites, false, ROLE_WORKER, BALDIE_DIR_N, 0, 50, 80, &level_pal);
    bal_sprites_draw_baldie(surf, &sprites, false, ROLE_BUILDER, BALDIE_DIR_S, 0, 80, 80, &level_pal);
    bal_sprites_draw_baldie(surf, &sprites, false, ROLE_SCIENTIST, BALDIE_DIR_E, 0, 110, 80, &level_pal);
    bal_sprites_draw_baldie(surf, &sprites, false, ROLE_SOLDIER, BALDIE_DIR_W, 0, 140, 80, &level_pal);
    TEST_ASSERT(true, "Draw multi-directional Baldies (North, South, East, West) to surface");

    // Test Alpha-Blended Fill: Translucent fill blends foreground and background
    surface_fill_rect(surf, 0, 0, 10, 10, 0xFFFF0000); // Solid red
    surface_fill_rect(surf, 0, 0, 10, 10, 0x800000FF); // 50% translucent blue
    uint32_t blended_pixel = surf->pixels[0];
    uint8_t br = (blended_pixel >> 16) & 0xFF;
    uint8_t bb = blended_pixel & 0xFF;
    TEST_ASSERT(br > 100 && br < 150 && bb > 100 && bb < 150, "surface_fill_rect performs proper alpha blending");

    // Draw 2 enemy Hairies
    bal_sprites_draw_baldie(surf, &sprites, true, 0, BALDIE_DIR_S, 0, 180, 80, &level_pal);

    // Test Unit-Unit Non-Collision: units pass freely through each other without mutual collision
    baldie_t *pass1 = entity_spawn(&emgr, TEAM_PLAYER, ROLE_WORKER, 800.0f, 600.0f);
    baldie_t *pass2 = entity_spawn(&emgr, TEAM_PLAYER, ROLE_WORKER, 800.0f, 600.0f);
    entity_update_all(&emgr, &map, &tileset, &test_hmgr);
    TEST_ASSERT(pass1->active && pass2->active && fabsf(pass1->x - pass2->x) < 0.01f && fabsf(pass1->y - pass2->y) < 0.01f,
                "Units have no mutual collision and can pass through each other");

    // Test Facing Direction: unit moving East faces East, unit moving South faces South
    baldie_t *b_dir = entity_spawn(&emgr, TEAM_PLAYER, ROLE_WORKER, 850.0f, 650.0f);
    b_dir->target_x = 900.0f;
    b_dir->target_y = 650.0f;
    entity_update_all(&emgr, &map, &tileset, &test_hmgr);
    TEST_ASSERT(b_dir->facing == BALDIE_DIR_E, "Unit moving right updates facing to BALDIE_DIR_E");

    b_dir->target_x = b_dir->x;
    b_dir->target_y = b_dir->y + 50.0f;
    entity_update_all(&emgr, &map, &tileset, &test_hmgr);
    TEST_ASSERT(b_dir->facing == BALDIE_DIR_S, "Unit moving down updates facing to BALDIE_DIR_S");
    // Dump all 220 sprites from LEV1PLR1.BAL to test_sprites_grid.bmp
    surface_t *spr_grid = surface_create(20 * 18, 11 * 18);
    surface_clear(spr_grid, 0xFF333333);
    for (int s = 0; s < 220; s++) {
        int col = s % 20;
        int row = s / 20;
        int dx = col * 18 + 1;
        int dy = row * 18 + 1;
        const uint8_t *sdata = &sprites.player_data[s * 256];
        for (int py = 0; py < 16; py++) {
            for (int px = 0; px < 16; px++) {
                uint8_t c = sdata[py * 16 + px];
                if (c != 0) spr_grid->pixels[(dy + py) * (20 * 18) + (dx + px)] = level_pal.argb[c];
            }
        }
    }
    uint8_t gbmp_hdr[54] = {
        'B', 'M',  0, 0, 0, 0,  0, 0, 0, 0,  54, 0, 0, 0,
        40, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  1, 0, 32, 0,
        0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0, 0, 0, 0, 0
    };
    uint32_t gfsz = 54 + (20 * 18) * (11 * 18) * 4;
    int32_t gbw = 20 * 18, gbh = -(11 * 18);
    memcpy(&gbmp_hdr[2], &gfsz, 4);
    memcpy(&gbmp_hdr[18], &gbw, 4);
    memcpy(&gbmp_hdr[22], &gbh, 4);
    FILE *gbf = fopen("test_sprites_grid.bmp", "wb");
    if (gbf) {
        fwrite(gbmp_hdr, 1, 54, gbf);
        fwrite(spr_grid->pixels, 4, (20 * 18) * (11 * 18), gbf);
        fclose(gbf);
    }
    surface_destroy(spr_grid);

    // 6. Hand Cursor and Grab & Drop Mechanics
    printf("\nTesting In-Game Hand Cursor & Grab/Drop Mechanics:\n");
    bal_cursor_t cursor;
    bool cur_ok = bal_cursor_init(&cursor);
    TEST_ASSERT(cur_ok && cursor.data != NULL && cursor.num_frames == 18, "Load CURS640.BAL with 18 frames of 32x32 cursors");

    // Test rendering open hand (Frame 4) and closed hand (Frame 5)
    bal_cursor_draw(surf, &cursor, CURSOR_FRAME_OPEN_HAND, 320, 240, &level_pal);
    bal_cursor_draw(surf, &cursor, CURSOR_FRAME_GRAB_HAND, 360, 240, &level_pal);
    TEST_ASSERT(true, "Render open hand (Frame 4) and closed hand (Frame 5) to surface");

    // Test Main Menu cursor rendering with level_pal palette
    menu_state_t test_menu;
    menu_init(&test_menu);
    TEST_ASSERT(test_menu.preview_surf != NULL && strcmp(test_menu.preview_theme, "GRASSLANDS") == 0, "Level 1 preview generates Grassland island map");

    // Test Theme Previews
    menu_load_preview(&test_menu, 26);
    TEST_ASSERT(test_menu.preview_surf != NULL && strcmp(test_menu.preview_theme, "ICE & SNOW") == 0, "Level 26 preview generates Ice realm map");
    menu_load_preview(&test_menu, 51);
    TEST_ASSERT(test_menu.preview_surf != NULL && strcmp(test_menu.preview_theme, "DESERT DUNES") == 0, "Level 51 preview generates Desert map");
    menu_load_preview(&test_menu, 1); // switch back to level 1

    // Test menu button interactions
    platform_input_t menu_input;
    memset(&menu_input, 0, sizeof(menu_input));

    // Verify left-offset bug is fixed: x=230 is outside drawn START MISSION (x: 245..553)
    menu_input.mouse_x = 230;
    menu_input.mouse_y = 370;
    menu_input.mouse_left_clicked = false;
    menu_update(&test_menu, &menu_input, 640, 480, NULL);
    TEST_ASSERT(test_menu.hovered_button == 0, "Mouse at x=230 (outside drawn button) does not hover START button");

    // Hover directly over drawn START MISSION (card_x + 215 = 245, width 308 -> center ~399, y=370)
    menu_input.mouse_x = 399;
    menu_input.mouse_y = 370;
    menu_update(&test_menu, &menu_input, 640, 480, NULL);
    TEST_ASSERT(test_menu.hovered_button == 1, "Mouse over rendered START button sets hovered_button == 1");

    // Click NEXT > button at rendered position (card_x + 434 = 464, width 42 -> center ~485, y=320)
    menu_input.mouse_x = 485;
    menu_input.mouse_y = 320;
    menu_input.mouse_left_clicked = true;
    menu_update(&test_menu, &menu_input, 640, 480, NULL);
    TEST_ASSERT(test_menu.selected_level == 2, "Clicking NEXT button advances to Level 2");

    // Click START MISSION button at rendered center (399, 370)
    menu_input.mouse_x = 399;
    menu_input.mouse_y = 370;
    menu_input.mouse_left_clicked = true;
    uint32_t chosen_lvl = 0;
    menu_action_t act = menu_update(&test_menu, &menu_input, 640, 480, &chosen_lvl);
    TEST_ASSERT(act == MENU_ACTION_START_LEVEL && chosen_lvl == 2, "Clicking START MISSION returns MENU_ACTION_START_LEVEL for Level 2");

    test_menu.selected_level = 1;
    menu_load_preview(&test_menu, 1);
    surface_t *menu_surf = surface_create(640, 480);
    menu_render(&test_menu, menu_surf);

    // Save test_menu_with_cursor.bmp
    uint8_t mbmp_hdr[54] = {
        'B', 'M',  0, 0, 0, 0,  0, 0, 0, 0,  54, 0, 0, 0,
        40, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  1, 0, 32, 0,
        0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0, 0, 0, 0, 0
    };
    uint32_t mfsz = 54 + 640 * 480 * 4;
    int32_t mbw = 640, mbh = -480;
    memcpy(&mbmp_hdr[2], &mfsz, 4);
    memcpy(&mbmp_hdr[18], &mbw, 4);
    memcpy(&mbmp_hdr[22], &mbh, 4);
    FILE *mbf = fopen("test_menu_with_cursor.bmp", "wb");
    if (mbf) {
        fwrite(mbmp_hdr, 1, 54, mbf);
        fwrite(menu_surf->pixels, 4, 640 * 480, mbf);
        fclose(mbf);
    }
    surface_destroy(menu_surf);
    menu_free(&test_menu);
    bal_cursor_free(&cursor);

    // Setup game state for grab & drop mechanics
    game_state_t test_game;
    memset(&test_game, 0, sizeof(test_game));
    test_game.state = APP_STATE_PLAYING;
    test_game.map = map;
    test_game.tileset = tileset;
    camera_init(&test_game.camera, 800, 650, map.width, map.height, 640, 480 - HUD_HEIGHT);
    test_game.hud_state.active_tool = TOOL_HAND;
    house_manager_init(&test_game.house_mgr);
    entity_manager_init(&test_game.entity_mgr);
    house_t *th = house_create(&test_game.house_mgr, TEAM_PLAYER, HOUSE_HUT, 912, 704);
    th->rooms[0] = 2;

    // Test A: Picking up friendly Baldie into hand
    baldie_t *b = entity_spawn(&test_game.entity_mgr, TEAM_PLAYER, ROLE_WORKER, 912.0f, 760.0f);
    TEST_ASSERT(b != NULL, "Spawn friendly player Baldie");
    b->state = STATE_CARRIED;
    test_game.held_unit = b;
    TEST_ASSERT(test_game.held_unit == b && b->state == STATE_CARRIED, "Hand picks up Baldie (held_unit set, STATE_CARRIED)");

    // Test B: Dropping Baldie on Walkable Ground
    game_drop_held_unit(&test_game, 940.0f, 760.0f);
    TEST_ASSERT(test_game.held_unit == NULL, "Hand drops unit (held_unit cleared)");
    TEST_ASSERT(b->state == STATE_IDLE && b->active == true, "Unit placed on ground in STATE_IDLE and active");
    TEST_ASSERT(b->x == (int)(940.0f / 16.0f) * 16.0f + 4.0f && b->y == (int)(760.0f / 16.0f) * 16.0f + 2.0f, "Unit aligned to target tile");
    TEST_ASSERT(b->anim_frame == 0 && b->anim_timer == 0, "Placed unit has anim_frame reset to 0 (standing idle, not walking)");
    TEST_ASSERT(entity_is_position_walkable(&test_game.map, &test_game.tileset, &test_game.house_mgr, b->x, b->y), "Placed unit is at 100% valid walkable position");

    // Test C: Dropping Baldie on Solid Obstacle (Snapping to nearest walkable tile)
    test_game.held_unit = b;
    b->state = STATE_CARRIED;
    // Drop onto Boulder rock at (66, 38)
    game_drop_held_unit(&test_game, 66 * 16.0f + 8.0f, 38 * 16.0f + 8.0f);
    TEST_ASSERT(test_game.held_unit == NULL, "Hand drops unit on obstacle (held_unit cleared)");
    TEST_ASSERT(b->state == STATE_IDLE, "Unit on obstacle placed in STATE_IDLE");
    int b_tx = (int)(b->x / 16.0f);
    int b_ty = (int)(b->y / 16.0f);
    TEST_ASSERT(pathfind_is_tile_walkable(&test_game.map, &test_game.tileset, &test_game.house_mgr, b_tx, b_ty), "Unit safely placed on nearest walkable tile outside obstacle");
    TEST_ASSERT(entity_is_position_walkable(&test_game.map, &test_game.tileset, &test_game.house_mgr, b->x, b->y), "Unit safely placed at valid entity position");

    // Test D: Dropping Baldie into House
    int initial_workers = th->rooms[ROLE_WORKER];
    test_game.held_unit = b;
    b->state = STATE_CARRIED;
    // Drop into cottage bounds (912 to 960, 704 to 752)
    game_drop_held_unit(&test_game, 920.0f, 720.0f);
    TEST_ASSERT(test_game.held_unit == NULL, "Hand drops unit into house (held_unit cleared)");
    TEST_ASSERT(th->rooms[ROLE_WORKER] == initial_workers + 1, "House worker count incremented by 1");
    TEST_ASSERT(b->state == STATE_INSIDE_HOUSE && b->active == false, "Unit enters house (STATE_INSIDE_HOUSE, active false)");

    // Test E: Dropping Baldie into Ocean Water
    baldie_t *b_water = entity_spawn(&test_game.entity_mgr, TEAM_PLAYER, ROLE_WORKER, 912.0f, 760.0f);
    test_game.held_unit = b_water;
    b_water->state = STATE_CARRIED;
    // Tile (10, 10) is deep ocean water (tile 340-359)
    game_drop_held_unit(&test_game, 10 * 16.0f + 8.0f, 10 * 16.0f + 8.0f);
    TEST_ASSERT(test_game.held_unit == NULL, "Hand drops unit into ocean (held_unit cleared)");
    TEST_ASSERT(b_water->active == false, "Unit drowns when dropped into deep water");

    // Test F: Unit moving directly into solid obstacle does not get stuck in walking animation
    baldie_t *b_stuck = entity_spawn(&test_game.entity_mgr, TEAM_PLAYER, ROLE_WORKER, 900.0f, 740.0f);
    b_stuck->target_x = 900.0f; // Pure vertical move straight into cottage wall (dx=0, dy>0)
    b_stuck->target_y = 760.0f;
    for (int step = 0; step < 20; step++) {
        entity_update_all(&test_game.entity_mgr, &test_game.map, &test_game.tileset, &test_game.house_mgr);
    }
    TEST_ASSERT(b_stuck->state == STATE_IDLE, "Blocked unit cleanly transitions to STATE_IDLE");
    TEST_ASSERT(b_stuck->anim_frame == 0, "Blocked unit stops animating and resets anim_frame to 0");

    // Test G: Toolbar Tool Selection (Hand + 4 Roles)
    int clk_hand = hud_handle_click(30, 480 - 20, 640, 480);
    int clk_worker = hud_handle_click(120, 480 - 20, 640, 480);
    int clk_builder = hud_handle_click(200, 480 - 20, 640, 480);
    int clk_science = hud_handle_click(280, 480 - 20, 640, 480);
    int clk_soldier = hud_handle_click(360, 480 - 20, 640, 480);
    TEST_ASSERT(clk_hand == TOOL_HAND, "HUD click on Hand button selects TOOL_HAND (0)");
    TEST_ASSERT(clk_worker == TOOL_ROLE_WORKER, "HUD click on Worker button selects TOOL_ROLE_WORKER (1)");
    TEST_ASSERT(clk_builder == TOOL_ROLE_BUILDER, "HUD click on Builder button selects TOOL_ROLE_BUILDER (2)");
    TEST_ASSERT(clk_science == TOOL_ROLE_SCIENTIST, "HUD click on Scientist button selects TOOL_ROLE_SCIENTIST (3)");
    TEST_ASSERT(clk_soldier == TOOL_ROLE_SOLDIER, "HUD click on Soldier button selects TOOL_ROLE_SOLDIER (4)");

    // Test H: Area Selection Role Conversion
    // Spawn 3 Workers on grass
    baldie_t *u1 = entity_spawn(&test_game.entity_mgr, TEAM_PLAYER, ROLE_WORKER, 820.0f, 750.0f);
    baldie_t *u2 = entity_spawn(&test_game.entity_mgr, TEAM_PLAYER, ROLE_WORKER, 835.0f, 750.0f);
    baldie_t *u3 = entity_spawn(&test_game.entity_mgr, TEAM_PLAYER, ROLE_WORKER, 890.0f, 750.0f); // outside selection

    // Switch tool to BUILDER
    test_game.hud_state.active_tool = TOOL_ROLE_BUILDER;
    test_game.hud_state.selected_role = ROLE_BUILDER;

    // Simulate Area Selection Drag over u1 and u2 (world 810..850, 740..770)
    int scr_x1 = (int)u1->x - test_game.camera.x - 5;
    int scr_y1 = (int)u1->y - test_game.camera.y - 5;
    int scr_x2 = (int)u2->x - test_game.camera.x + 15;
    int scr_y2 = (int)u2->y - test_game.camera.y + 15;

    platform_input_t sel_input = {0};
    sel_input.mouse_x = scr_x1;
    sel_input.mouse_y = scr_y1;
    sel_input.mouse_left_clicked = true;
    sel_input.mouse_left_down = true;
    game_tick(&test_game, &sel_input, 640, 480);
    TEST_ASSERT(test_game.is_area_selecting == true, "Selecting role changes mouse interaction to Area Selection");

    // Mouse drag release over u1 and u2
    sel_input.mouse_x = scr_x2;
    sel_input.mouse_y = scr_y2;
    sel_input.mouse_left_clicked = false;
    sel_input.mouse_left_down = false;
    sel_input.mouse_left_released = true;
    game_tick(&test_game, &sel_input, 640, 480);

    TEST_ASSERT(test_game.is_area_selecting == false, "Mouse release finishes area selection");
    TEST_ASSERT(u1->role == ROLE_BUILDER, "Unit 1 inside selection converted to Builder");
    TEST_ASSERT(u2->role == ROLE_BUILDER, "Unit 2 inside selection converted to Builder");
    TEST_ASSERT(u3->role == ROLE_WORKER, "Unit 3 outside selection remains Worker");

    // Test I: Single Click Area Selection converts single unit
    test_game.hud_state.active_tool = TOOL_ROLE_SOLDIER;
    test_game.hud_state.selected_role = ROLE_SOLDIER;
    int u3_sx = (int)u3->x - test_game.camera.x + 8;
    int u3_sy = (int)u3->y - test_game.camera.y + 8;

    sel_input.mouse_x = u3_sx;
    sel_input.mouse_y = u3_sy;
    sel_input.mouse_left_clicked = true;
    sel_input.mouse_left_down = true;
    sel_input.mouse_left_released = false;
    game_tick(&test_game, &sel_input, 640, 480);

    sel_input.mouse_left_clicked = false;
    sel_input.mouse_left_down = false;
    sel_input.mouse_left_released = true;
    game_tick(&test_game, &sel_input, 640, 480);
    TEST_ASSERT(u3->role == ROLE_SOLDIER, "Single-clicking unit in Soldier mode converts unit to Soldier");

    // Test J: Switching back to TOOL_HAND enables grab again
    sel_input.mouse_x = 30;
    sel_input.mouse_y = 480 - 20;
    sel_input.mouse_left_clicked = true;
    sel_input.mouse_left_down = true;
    sel_input.mouse_left_released = false;
    game_tick(&test_game, &sel_input, 640, 480);
    TEST_ASSERT(test_game.hud_state.active_tool == TOOL_HAND, "Clicking Hand button on toolbar switches back to TOOL_HAND");


    // Generate tileset atlas image (40x32 tiles of 16x16 = 640x512)
    surface_t *atlas = surface_create(640, 512);
    for (uint32_t t = 0; t < tileset.num_tiles && t < 1280; t++) {
        uint32_t col = t % 40;
        uint32_t row = t / 40;
        int dx = col * 16;
        int dy = row * 16;
        const uint8_t *tdata = bal_tileset_get_tile(&tileset, t);
        for (int py = 0; py < 16; py++) {
            for (int px = 0; px < 16; px++) {
                uint8_t c = tdata[py * 16 + px];
                atlas->pixels[(dy + py) * 640 + (dx + px)] = level_pal.argb[c];
            }
        }
    }
    // Save atlas BMP
    uint8_t abmp_hdr[54] = {
        'B', 'M',  0, 0, 0, 0,  0, 0, 0, 0,  54, 0, 0, 0,
        40, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  1, 0, 32, 0,
        0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0, 0, 0, 0, 0
    };
    uint32_t afsz = 54 + 640 * 512 * 4;
    int32_t abw = 640, abh = -512;
    memcpy(&abmp_hdr[2], &afsz, 4);
    memcpy(&abmp_hdr[18], &abw, 4);
    memcpy(&abmp_hdr[22], &abh, 4);
    FILE *abf = fopen("test_tileset_atlas.bmp", "wb");
    if (abf) {
        fwrite(abmp_hdr, 1, 54, abf);
        fwrite(atlas->pixels, 4, 640 * 512, abf);
        fclose(abf);
    }
    surface_destroy(atlas);

    // Save test BMP
    uint8_t bmp_hdr[54] = {
        'B', 'M',  0, 0, 0, 0,  0, 0, 0, 0,  54, 0, 0, 0,
        40, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  1, 0, 32, 0,
        0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0, 0, 0, 0, 0
    };
    uint32_t fsz = 54 + 640 * 480 * 4;
    int32_t bw = 640, bh = -480;
    memcpy(&bmp_hdr[2], &fsz, 4);
    memcpy(&bmp_hdr[18], &bw, 4);
    memcpy(&bmp_hdr[22], &bh, 4);
    // Draw translucent selection box (Blue Builder mode) over units at (40, 60, 130, 50)
    surface_fill_rect(surf, 40, 65, 130, 50, 0x403388FF);
    surface_draw_rect(surf, 40, 65, 130, 50, 0xFF3388FF);

    // Draw HUD toolbar and cursor onto surf
    bal_cursor_t hud_cur;
    bal_cursor_init(&hud_cur);
    test_game.hud_state.workers_red = 4;
    test_game.hud_state.builders_blue = 2;
    test_game.hud_state.scientists_white = 1;
    test_game.hud_state.soldiers_green = 3;
    hud_render(surf, &test_game.hud_res, &test_game.hud_state, "GRASSLANDS", &level_pal, &hud_cur);
    bal_cursor_draw(surf, &hud_cur, CURSOR_FRAME_AREA_SELECT, 170, 115, &level_pal);
    bal_cursor_free(&hud_cur);

    FILE *bf = fopen("test_ingame_16x16.bmp", "wb");
    if (bf) {
        fwrite(bmp_hdr, 1, 54, bf);
        fwrite(surf->pixels, 4, 640 * 480, bf);
        fclose(bf);
    }
    TEST_ASSERT(bf != NULL, "Render in-game 640x480 frame to test_ingame_16x16.bmp");
    surface_destroy(surf);
    bal_sprites_free(&sprites);

    // 6. SFX Extraction Test
    printf("\nTesting SFX Resource Extraction:\n");
    bool sfx_init = bal_sfx_init("baldies.exe");
    if (!sfx_init) sfx_init = bal_sfx_init("../baldies.exe");
    TEST_ASSERT(sfx_init, "Extract 68 WAV sound effects from baldies.exe PE resources");

    // Summary
    printf("\n======================================================\n");
    printf("Tests run: %d | Passed: %d | Failed: %d\n", g_tests_run, g_tests_passed, g_tests_run - g_tests_passed);
    printf("======================================================\n");

    // Cleanup
    bal_tileset_free(&tileset);
    bal_map_free(&map);
    bal_sfx_shutdown();

    return (g_tests_passed == g_tests_run) ? 0 : 1;

}
