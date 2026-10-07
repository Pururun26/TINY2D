#include "tiny2d_audio.h"
#include "tiny2d_conf.h"
#include "raylib.h"

#include <stdio.h>

// ============================================================
// Состояние
// ============================================================

static Sound sfx_slots[TINY2D_MAX_SFX];
static bool  sfx_used [TINY2D_MAX_SFX];

static Music music_slots[TINY2D_MAX_MUSIC];
static bool  music_used [TINY2D_MAX_MUSIC];
static int   current_music = -1;

// ============================================================
// Инициализация
// ============================================================

void tiny2d_audio_init(void) {
    InitAudioDevice();
    if (!IsAudioDeviceReady()) {
        TraceLog(LOG_ERROR, "TINY2D: Audio device failed to initialize!");
        return;
    }

    // SFX: 0.wav, 1.wav, 2.wav, ... до первой неудачи
    for (int i = 0; i < TINY2D_MAX_SFX; i++) {
        char path[1024];
        snprintf(path, sizeof(path), "%s%s%d.wav",
                 GetApplicationDirectory(), TINY2D_SOUNDS_PATH, i);
        
        // Проверяем перед загрузкой
        if (!FileExists(path)) continue;

        Sound s = LoadSound(path);
        if (s.frameCount == 0) continue;   // нет файла — пропускаем

        sfx_slots[i] = s;
        sfx_used[i]  = true;
    }

    // Music: 0.ogg, 1.ogg, ... до первой неудачи
    for (int i = 0; i < TINY2D_MAX_MUSIC; i++) {
        char path[1024];
        snprintf(path, sizeof(path), "%s%s%d.ogg",
                 GetApplicationDirectory(), TINY2D_MUSIC_PATH, i);
        
        // Проверяем перед загрузкой
        if (!FileExists(path)) continue;

        Music m = LoadMusicStream(path);
        if (m.ctxData == NULL) continue;

        music_slots[i] = m;
        music_used[i]  = true;
    }

    TraceLog(LOG_INFO, "TINY2D: Audio ready.");
}

void tiny2d_audio_close(void) {
    for (int i = 0; i < TINY2D_MAX_SFX; i++) {
        if (sfx_used[i]) UnloadSound(sfx_slots[i]);
    }
    for (int i = 0; i < TINY2D_MAX_MUSIC; i++) {
        if (music_used[i]) UnloadMusicStream(music_slots[i]);
    }
    CloseAudioDevice();
}

// ============================================================
// SFX
// ============================================================
void sfx_ex(int index, float volume) {
    if (index < 0 || index >= TINY2D_MAX_SFX) return;
    if (!sfx_used[index]) return;
    SetSoundVolume(sfx_slots[index], volume);
    PlaySound(sfx_slots[index]);
}

void sfx(int index) {
    sfx_ex(index, 1.0f);
}

void sfx_stop_all(void) {
    for (int i = 0; i < TINY2D_MAX_SFX; i++) {
        if (sfx_used[i]) StopSound(sfx_slots[i]);
    }
}

// ============================================================
// Music
// ============================================================

void music(int index) {
    if (index < 0 || index >= TINY2D_MAX_MUSIC) return;
    if (!music_used[index]) return;

    // Уже играет этот трек — ничего не делаем
    if (current_music == index) return;

    // Останавливаем текущую, если была
    if (current_music >= 0) {
        StopMusicStream(music_slots[current_music]);
    }

    current_music = index;
    music_slots[index].looping = true;
    PlayMusicStream(music_slots[index]);
}

void music_stop(void) {
    if (current_music < 0) return;
    StopMusicStream(music_slots[current_music]);
    current_music = -1;
}

void music_pause(void) {
    if (current_music < 0) return;
    PauseMusicStream(music_slots[current_music]);
}

void music_resume(void) {
    if (current_music < 0) return;
    ResumeMusicStream(music_slots[current_music]);
}

void music_volume(float v) {
    if (current_music < 0) return;
    if (v < 0.0f) v = 0.0f;
    if (v > 1.0f) v = 1.0f;
    SetMusicVolume(music_slots[current_music], v);
}

void music_update(void) {
    if (current_music < 0) return;
    UpdateMusicStream(music_slots[current_music]);
}