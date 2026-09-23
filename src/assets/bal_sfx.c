#include "bal_sfx.h"
#include "asset_path.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <windows.h>
#include <mmsystem.h>

static HMODULE g_res_module = NULL;
static bal_sound_t g_sounds[MAX_SFX_COUNT + 1];

bool bal_sfx_init(const char *exe_path) {
    if (!exe_path) {
        exe_path = "baldies.exe";
    }

    char resolved[MAX_PATH];
    if (asset_resolve_path(exe_path, resolved, sizeof(resolved))) {
        g_res_module = LoadLibraryExA(resolved, NULL, LOAD_LIBRARY_AS_DATAFILE);
    }

    if (!g_res_module) {
        g_res_module = LoadLibraryExA(exe_path, NULL, LOAD_LIBRARY_AS_DATAFILE);
    }

    if (!g_res_module) {
        return false;
    }

    // Cache sound pointers
    for (int i = 1; i <= MAX_SFX_COUNT; i++) {
        char res_name[16];
        snprintf(res_name, sizeof(res_name), "SD%03d", i);

        HRSRC hRes = FindResourceA(g_res_module, res_name, "WAV");
        if (hRes) {
            HGLOBAL hData = LoadResource(g_res_module, hRes);
            if (hData) {
                g_sounds[i].data = (const uint8_t*)LockResource(hData);
                g_sounds[i].size = SizeofResource(g_res_module, hRes);
            }
        }
    }

    return true;
}

void bal_sfx_play(uint32_t sfx_id) {
    if (sfx_id < 1 || sfx_id > MAX_SFX_COUNT) return;
    if (!g_sounds[sfx_id].data || g_sounds[sfx_id].size == 0) return;

    PlaySoundA((LPCSTR)g_sounds[sfx_id].data, NULL, SND_MEMORY | SND_ASYNC | SND_NODEFAULT);
}

void bal_sfx_stop_all(void) {
    PlaySoundA(NULL, NULL, 0);
}

void bal_sfx_shutdown(void) {
    bal_sfx_stop_all();
    if (g_res_module) {
        FreeLibrary(g_res_module);
        g_res_module = NULL;
    }
}

#else

bool bal_sfx_init(const char *exe_path) { (void)exe_path; return false; }
void bal_sfx_play(uint32_t sfx_id) { (void)sfx_id; }
void bal_sfx_stop_all(void) {}
void bal_sfx_shutdown(void) {}

#endif
