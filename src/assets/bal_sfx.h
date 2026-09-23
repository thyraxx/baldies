#ifndef BAL_SFX_H
#define BAL_SFX_H

#include <stdint.h>
#include <stdbool.h>

#define MAX_SFX_COUNT 68

typedef struct {
    const uint8_t *data;
    uint32_t size;
} bal_sound_t;

bool bal_sfx_init(const char *exe_path);
void bal_sfx_play(uint32_t sfx_id);
void bal_sfx_stop_all(void);
void bal_sfx_shutdown(void);

#endif // BAL_SFX_H
