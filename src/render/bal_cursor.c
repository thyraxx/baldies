#include "bal_cursor.h"
#include "assets/asset_path.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

bool bal_cursor_init(bal_cursor_t *cur) {
    if (!cur) return false;
    memset(cur, 0, sizeof(bal_cursor_t));

    FILE *f = asset_open_file("BALS/CURS640.BAL", "rb");
    if (!f) {
        f = asset_open_file("bals/curs640.bal", "rb");
    }
    if (!f) return false;

    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    fseek(f, 0, SEEK_SET);

    if (sz <= 0) {
        fclose(f);
        return false;
    }

    cur->data = (uint8_t*)malloc(sz);
    if (!cur->data) {
        fclose(f);
        return false;
    }

    size_t nread = fread(cur->data, 1, sz, f);
    fclose(f);

    if (nread != (size_t)sz) {
        free(cur->data);
        cur->data = NULL;
        return false;
    }

    cur->num_frames = (uint32_t)(sz / 1024);
    return true;
}

void bal_cursor_free(bal_cursor_t *cur) {
    if (cur && cur->data) {
        free(cur->data);
        cur->data = NULL;
        cur->num_frames = 0;
    }
}

void bal_cursor_draw(surface_t *dest, const bal_cursor_t *cur, int frame_idx, int x, int y, const bal_palette_t *palette) {
    if (!dest || !cur || !cur->data || !palette) return;
    if (frame_idx < 0 || frame_idx >= (int)cur->num_frames) return;

    const uint8_t *cdata = &cur->data[frame_idx * 1024];

    for (int py = 0; py < 32; py++) {
        int dy = y + py;
        if (dy < 0 || dy >= (int)dest->height) continue;

        for (int px = 0; px < 32; px++) {
            int dx = x + px;
            if (dx < 0 || dx >= (int)dest->width) continue;

            uint8_t idx = cdata[py * 32 + px];
            if (idx == 0) continue; // 0 is transparent

            dest->pixels[dy * dest->width + dx] = palette->argb[idx];
        }
    }
}
