#pragma once
#include <stdbool.h>

void tiny2d_audio_init(void);      // грузит все N.wav / N.ogg
void tiny2d_audio_close(void);

// SFX
void sfx(int index);
void sfx_ex(int index, float volume);
void sfx_stop_all(void);

// Music
void music(int index);
void music_stop(void);
void music_pause(void);
void music_resume(void);
void music_volume(float v);
void music_update(void);