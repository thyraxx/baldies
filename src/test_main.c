#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include "assets/bal_palette.h"
#include "assets/bal_tiles.h"
#include "assets/bal_map.h"
#include "assets/bal_sprites.h"
#include "assets/bal_sfx.h"
#include "assets/bal_midi.h"
#include "assets/asset_path.h"
#include "render/map_renderer.h"
#include "game/house.h"



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

    // Draw 4 player Baldies with 4 different roles (Red, Blue, White, Green)
    for (int r = 0; r < 4; r++) {
        bal_sprites_draw_baldie(surf, &sprites, false, r, r, 50 + r * 30, 80, &level_pal);
    }
    // Draw 2 enemy Hairies
    bal_sprites_draw_baldie(surf, &sprites, true, 0, 0, 180, 80, &level_pal);
    bal_sprites_draw_baldie(surf, &sprites, true, 0, 1, 210, 80, &level_pal);


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
