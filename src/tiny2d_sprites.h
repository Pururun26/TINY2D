#pragma once
#include <stdint.h>
#include <stdbool.h>
#include "raylib.h"

// Загрузить спрайт-лист (PNG 256×256) и построить индексный буфер.
void tiny2d_load_spritesheet(const char *filepath);

// Перестроить текстуру из индексов и текущей палитры.
// Вызывается автоматически при set_pal(). Можно звать вручную.
void tiny2d_rebuild_spritesheet(void);

// Отрисовать спрайт 8×8 по индексу n.
void spr(int16_t n, int16_t x, int16_t y);

// Отрисовать блок w×h спрайтов с флипом.
void spr_pro(int16_t n, int16_t x, int16_t y,
             uint8_t w, uint8_t h,
             bool flip_x, bool flip_y);