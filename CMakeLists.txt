cmake_minimum_required(VERSION 3.16)
project(BaldiesC VERSION 1.0.0 LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Windows Win32 compilation settings
if (MSVC)
    add_compile_options(/W3 /utf-8 /D_CRT_SECURE_NO_WARNINGS)
endif()

include_directories(src)

set(ASSET_SOURCES
    src/assets/asset_path.c
    src/assets/bal_palette.c
    src/assets/bal_tiles.c
    src/assets/bal_map.c
    src/assets/bal_sprites.c
    src/assets/bal_sfx.c
    src/assets/bal_midi.c
)

set(RENDER_SOURCES
    src/render/surface.c
    src/render/map_renderer.c
)

set(GAME_SOURCES
    src/game/camera.c
    src/game/hud.c
    src/game/menu.c
    src/game/entities.c
    src/game/house.c
    src/game/game_loop.c
)

set(PLATFORM_SOURCES
    src/platform/platform_win32.c
)

add_executable(baldies_c
    src/main.c
    ${ASSET_SOURCES}
    ${RENDER_SOURCES}
    ${GAME_SOURCES}
    ${PLATFORM_SOURCES}
)

if (WIN32)
    target_link_libraries(baldies_c PRIVATE gdi32 user32 winmm shell32)
endif()

# Separate unit test executable to verify asset decoding independently
add_executable(baldies_test
    src/test_main.c
    ${ASSET_SOURCES}
    ${RENDER_SOURCES}
)

if (WIN32)
    target_link_libraries(baldies_test PRIVATE gdi32 user32 winmm)
endif()
