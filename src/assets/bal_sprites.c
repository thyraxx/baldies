#include "bal_sprites.h"
#include "asset_path.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

bool bal_sprites_init(bal_sprites_t *sprites, uint32_t level_theme) {
    if (!sprites) return false;
    sprites->player_data = NULL;
    sprites->enemy_data = NULL;

    if (level_theme < 1 || level_theme > 5) level_theme = 1;

    char plr_path[64];
    snprintf(plr_path, sizeof(plr_path), "BALS/LEV%uPLR1.BAL", level_theme);
    FILE *f_plr = asset_open_file(plr_path, "rb");
    if (!f_plr) f_plr = asset_open_file("BALS/LEV1PLR1.BAL", "rb");

    if (f_plr) {
        sprites->player_data = (uint8_t*)malloc(SPRITE_SHEET_W * SPRITE_SHEET_H);
        if (sprites->player_data) {
            fread(sprites->player_data, 1, SPRITE_SHEET_W * SPRITE_SHEET_H, f_plr);
        }
        fclose(f_plr);
    }

    char nme_path[64];
    snprintf(nme_path, sizeof(nme_path), "BALS/LEV%uNME.BAL", level_theme);
    FILE *f_nme = asset_open_file(nme_path, "rb");
    if (!f_nme) f_nme = asset_open_file("BALS/LEV1NME.BAL", "rb");

    if (f_nme) {
        sprites->enemy_data = (uint8_t*)malloc(SPRITE_SHEET_W * SPRITE_SHEET_H);
        if (sprites->enemy_data) {
            fread(sprites->enemy_data, 1, SPRITE_SHEET_W * SPRITE_SHEET_H, f_nme);
        }
        fclose(f_nme);
    }

    return (sprites->player_data != NULL);
}

void bal_sprites_free(bal_sprites_t *sprites) {
    if (sprites) {
        if (sprites->player_data) {
            free(sprites->player_data);
            sprites->player_data = NULL;
        }
        if (sprites->enemy_data) {
            free(sprites->enemy_data);
            sprites->enemy_data = NULL;
        }
    }
}

void bal_sprites_draw_frame(surface_t *dest,
                            const bal_sprites_t *sprites,
                            bool is_enemy,
                            int role,
                            int sprite_idx,
                            bool flip_x,
                            int dest_x, int dest_y,
                            const bal_palette_t *palette) {
    if (!dest || !dest->pixels || !sprites || !palette) return;

    const uint8_t *sheet = is_enemy ? (sprites->enemy_data ? sprites->enemy_data : sprites->player_data) : sprites->player_data;
    if (!sheet) return;
    if (sprite_idx < 0 || sprite_idx >= TOTAL_SPRITES) return;

    const uint8_t *spr_data = &sheet[sprite_idx * 256];
    int dw = (int)dest->width;
    int dh = (int)dest->height;

    for (int py = 0; py < SPRITE_CELL_H; py++) {
        int dy = dest_y + py;
        if (dy < 0 || dy >= dh) continue;

        const uint8_t *srow = &spr_data[py * SPRITE_CELL_W];
        uint32_t *drow = &dest->pixels[dy * dw];

        for (int px = 0; px < SPRITE_CELL_W; px++) {
            int dx = dest_x + px;
            if (dx < 0 || dx >= dw) continue;

            int sx = flip_x ? (SPRITE_CELL_W - 1 - px) : px;
            uint8_t c = srow[sx];
            if (c == 0) continue; // Transparency

            uint32_t pixel_color = palette->bgra[c];

            // Role Overalls Recoloring for player Baldies
            if (!is_enemy) {
                if (c == 10 || c == 11 || c == 85) {
                    if (role == 1) { // Blue (Builder)
                        pixel_color = (c == 10) ? 0xFF3388FF : ((c == 11) ? 0xFF1144AA : 0xFF082255);
                    } else if (role == 2) { // White (Scientist)
                        pixel_color = (c == 10) ? 0xFFFFFFFF : ((c == 11) ? 0xFFCCCCCC : 0xFF777777);
                    } else if (role == 3) { // Green (Soldier)
                        pixel_color = (c == 10) ? 0xFF33DD33 : ((c == 11) ? 0xFF118811 : 0xFF084408);
                    }
                    // Role 0 (Worker) keeps original red!
                }
            }

            drow[dx] = pixel_color;
        }
    }
}

void bal_sprites_draw_baldie(surface_t *dest,
                             const bal_sprites_t *sprites,
                             bool is_enemy,
                             int role,
                             baldie_direction_t dir,
                             int anim_frame,
                             int dest_x, int dest_y,
                             const bal_palette_t *palette) {
    int frame = anim_frame % 8;
    int base_sprite = 0;
    bool flip_x = false;

    switch (dir) {
        case BALDIE_DIR_N:
            base_sprite = 0;   // Row 0: North walk (away/back)
            flip_x = false;
            break;
        case BALDIE_DIR_S:
            base_sprite = 40;  // Row 2: South walk (facing front)
            flip_x = false;
            break;
        case BALDIE_DIR_E:
            base_sprite = 20;  // Row 1: East walk (facing right)
            flip_x = false;
            break;
        case BALDIE_DIR_W:
            base_sprite = 20;  // Row 1 mirrored: West walk (facing left)
            flip_x = true;
            break;
        case BALDIE_DIR_SE:
            base_sprite = 60;  // Row 3: South-East walk (facing front-right)
            flip_x = false;
            break;
        case BALDIE_DIR_SW:
            base_sprite = 60;  // Row 3 mirrored: South-West walk (facing front-left)
            flip_x = true;
            break;
        case BALDIE_DIR_NE:
            base_sprite = 20;  // Side profile facing right/up
            flip_x = false;
            break;
        case BALDIE_DIR_NW:
            base_sprite = 20;  // Side profile facing left/up
            flip_x = true;
            break;
        default:
            base_sprite = 40;
            flip_x = false;
            break;
    }

    int sprite_idx = base_sprite + frame;
    bal_sprites_draw_frame(dest, sprites, is_enemy, role, sprite_idx, flip_x, dest_x, dest_y, palette);
}

