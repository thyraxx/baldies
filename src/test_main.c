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
