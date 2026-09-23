#include "bal_midi.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <windows.h>
#include <mmsystem.h>

static bool g_midi_playing = false;

static bool resolve_filepath(const char *subpath, char *out_full_path, size_t out_size) {
    const char *prefixes[] = { "", "../", "../../", "I:/Baldies/", NULL };
    for (int i = 0; prefixes[i] != NULL; i++) {
        char test_path[260];
        snprintf(test_path, sizeof(test_path), "%s%s", prefixes[i], subpath);
        DWORD attr = GetFileAttributesA(test_path);
        if (attr != INVALID_FILE_ATTRIBUTES && !(attr & FILE_ATTRIBUTE_DIRECTORY)) {
            GetFullPathNameA(test_path, (DWORD)out_size, out_full_path, NULL);
            return true;
        }
    }
    return false;
}

bool bal_midi_play(const char *filepath, bool loop) {
    bal_midi_stop();

    char full_path[260];
    if (!resolve_filepath(filepath, full_path, sizeof(full_path))) {
        return false;
    }

    char cmd[512];
    snprintf(cmd, sizeof(cmd), "open \"%s\" type sequencer alias bgm", full_path);
    MCIERROR err = mciSendStringA(cmd, NULL, 0, NULL);
    if (err != 0) {
        return false;
    }

    if (loop) {
        mciSendStringA("play bgm repeat", NULL, 0, NULL);
    } else {
        mciSendStringA("play bgm", NULL, 0, NULL);
    }

    g_midi_playing = true;
    return true;
}

void bal_midi_stop(void) {
    if (g_midi_playing) {
        mciSendStringA("stop bgm", NULL, 0, NULL);
        mciSendStringA("close bgm", NULL, 0, NULL);
        g_midi_playing = false;
    }
}

void bal_midi_pause(void) {
    if (g_midi_playing) {
        mciSendStringA("pause bgm", NULL, 0, NULL);
    }
}

void bal_midi_resume(void) {
    if (g_midi_playing) {
        mciSendStringA("resume bgm", NULL, 0, NULL);
    }
}

void bal_midi_set_volume(int volume_percent) {
    if (volume_percent < 0) volume_percent = 0;
    if (volume_percent > 100) volume_percent = 100;
    DWORD vol = (DWORD)((volume_percent / 100.0) * 0xFFFF);
    midiOutSetVolume(0, (vol << 16) | vol);
}

void bal_midi_shutdown(void) {
    bal_midi_stop();
}

#else

bool bal_midi_play(const char *filepath, bool loop) { (void)filepath; (void)loop; return false; }
void bal_midi_stop(void) {}
void bal_midi_pause(void) {}
void bal_midi_resume(void) {}
void bal_midi_set_volume(int volume_percent) { (void)volume_percent; }
void bal_midi_shutdown(void) {}

#endif
