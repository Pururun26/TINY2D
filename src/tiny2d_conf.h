#pragma once

// Настройки по умолчанию (пользователь может отредактировать)
// Размер окна (если не определены, используется автоматический расчёт)
#define TINY2D_WINDOW_WIDTH  640
#define TINY2D_WINDOW_HEIGHT 640
#define TINY2D_WIDTH   160
#define TINY2D_HEIGHT  160
#define TINY2D_TITLE   "TINY2D"
#define TINY2D_FPS     60


// Палитра
#define TINY2D_COLOR_COUNT 4


// Скрипт игры
#define TINY2D_MAIN_SCRIPT  "data/scripts/main.lua"


// Спрайты
#define TINY2D_SPRITE_SIZE          8      // 8×8 пикселей
#define TINY2D_SPRITE_SHEET_TILES   32     // 32 спрайта в строке
#define TINY2D_SPRITE_SHEET_W       256    // ширина листа в пикселях
#define TINY2D_SPRITE_SHEET_H       256    // высота листа

#define TINY2D_SPRITESHEET_PATH     "data/sprites/spritesheet.png"


// Шрифт
#define TINY2D_FONT_PATH    "data/sprites/font.png"


// Звук
#define TINY2D_SOUNDS_PATH  "data/sounds/"
#define TINY2D_MUSIC_PATH   "data/music/"

#define TINY2D_MAX_SFX     64    // 0.wav … 63.wav
#define TINY2D_MAX_MUSIC    8    // 0.ogg … 7.ogg

// Сохранение и загрузка
#define TINY2D_SAVE_SLOTS 64
/*

// Мышка
#define PICOLIB_USE_MOUSE       1


// Карта
#define PICOLIB_USE_MAP         1

// Размеры карты
#define MAP_ROWS     32
#define MAP_COLS     128

// Путь к файлу карты (если используется)
#define PICOLIB_MAP_FILE "map.csv"
*/