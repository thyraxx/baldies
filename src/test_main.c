#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include "assets/bal_palette.h"
#include "assets/bal_tiles.h"
#include "assets/bal_map.h"
#include "assets/bal_sfx.h"
#include "assets/bal_midi.h"
#include "assets/asset_path.h"

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
    TEST_ASSERT(tileset.num_tiles == 320, "Tileset contains exactly 320 tiles");
    const uint8_t *tile0 = bal_tileset_get_tile(&tileset, 0);
    TEST_ASSERT(tile0 != NULL, "Get tile 0 data");

    // 3. Map & Level Loading Test
    printf("\nTesting Map & Level Loading:\n");
    bal_map_t map;
    bool map_loaded = bal_map_load(1, &map);
    TEST_ASSERT(map_loaded, "Load Level 1 (GRASS1)");
    TEST_ASSERT(map.width == 116, "Map width is 116 tiles");
    TEST_ASSERT(map.height == 112, "Map height is 112 tiles");
    TEST_ASSERT(map.start_cam_x == 880 && map.start_cam_y == 688, "Camera spawn coordinates (880, 688)");
    uint16_t border_tile = bal_map_get_tile(&map, 0, 0);
    TEST_ASSERT(border_tile == 340, "Border tile index is 340 (0x0154)");

    // 4. SFX Extraction Test
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
