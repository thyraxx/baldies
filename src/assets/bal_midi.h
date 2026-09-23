#ifndef BAL_MIDI_H
#define BAL_MIDI_H

#include <stdbool.h>

bool bal_midi_play(const char *filepath, bool loop);
void bal_midi_stop(void);
void bal_midi_pause(void);
void bal_midi_resume(void);
void bal_midi_set_volume(int volume_percent); // 0 to 100
void bal_midi_shutdown(void);

#endif // BAL_MIDI_H
