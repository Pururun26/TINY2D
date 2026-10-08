#include <time.h>
#include "raylib.h"
#include "tiny2d_api.h"
#include "tiny2d_conf.h"
#include "tiny2d_sprites.h"

// --- Палитра ---
static Color palette[TINY2D_COLOR_COUNT] = {
    {0x42, 0x29, 0x36, 0xFF}, // 0: dark    #422936
    {0xA9, 0x60, 0x4C, 0xFF}, // 1: brown   #a9604c
    {0xDC, 0xA4, 0x56, 0xFF}, // 2: tan     #dca456
    {0xFF, 0xE4, 0xC2, 0xFF}, // 3: cream   #ffe4c2
};

// -- Палитра которую можно менять цвета местами
static const Color palette_default[TINY2D_COLOR_COUNT] = {
    {0x42, 0x29, 0x36, 0xFF},
    {0xA9, 0x60, 0x4C, 0xFF},
    {0xDC, 0xA4, 0x56, 0xFF},
    {0xFF, 0xE4, 0xC2, 0xFF},
};

Color tiny2d_get_palette(uint8_t idx) {
    return (idx < TINY2D_COLOR_COUNT) ? palette[idx] : palette[0];
}

void set_pal(uint8_t m0, uint8_t m1, uint8_t m2, uint8_t m3) {
    uint8_t map[TINY2D_COLOR_COUNT] = { m0, m1, m2, m3 };

    // Проверка границ
    for (int i = 0; i < TINY2D_COLOR_COUNT; i++) {
        if (map[i] >= TINY2D_COLOR_COUNT) map[i] = 0;
    }

    // Собираем новые цвета из СТАРЫХ (важно: из palette_default, а не из palette)
    Color next[TINY2D_COLOR_COUNT];
    for (int i = 0; i < TINY2D_COLOR_COUNT; i++) {
        next[i] = palette_default[map[i]];
    }

    for (int i = 0; i < TINY2D_COLOR_COUNT; i++) {
        palette[i] = next[i];
    }

    tiny2d_rebuild_spritesheet();
}

void reset_pal(void) {
    for (int i = 0; i < TINY2D_COLOR_COUNT; i++) {
        palette[i] = palette_default[i];
    }
    tiny2d_rebuild_spritesheet();
}

// --- Шрифт ---
static Font  pico_font;
static bool  font_loaded = false;
static float font_size = 5.0f;
static float font_spacing = 1.0f;

void tiny2d_load_font(const char* filepath) {
    Image img = LoadImage(filepath);
    if (img.data != NULL) {
        pico_font = LoadFontFromImage(img, MAGENTA, 32);
        UnloadImage(img);
        font_loaded = true;
        TraceLog(LOG_INFO, "TINY2D: Font '%s' loaded successfully.", filepath);
    } else {
        pico_font = GetFontDefault();
        font_size = 10.0f;
        font_loaded = false;
        TraceLog(LOG_WARNING, "TINY2D: Font '%s' not found. Using default.", filepath);
    }
}

// ============================================================
// C-функции API (используются Lua-биндингами)
// ============================================================

void cls(uint8_t color) {
    if (color < TINY2D_COLOR_COUNT) ClearBackground(palette[color]);
    else                             ClearBackground(palette[0]);
}

void print(const char* format, int16_t x, int16_t y, int color, ...) {
    va_list args;
    va_start(args, color);
    char buffer[256];
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);

    Color c = (color < TINY2D_COLOR_COUNT) ? palette[color] : palette[0];
    Vector2 pos = { (float)x, (float)y };
    DrawTextEx(pico_font, buffer, pos, font_size, font_spacing, c);
}

void fps(int16_t x, int16_t y, uint8_t color) {
    int value = GetFPS();
    print("FPS %d", x, y, color, value);
}

// --- ПРИМИТИВЫ РИСОВАНИЯ ---
void circ(int16_t x, int16_t y, int16_t r, uint8_t color) {
    DrawCircle(x, y, (float)r, tiny2d_get_palette(color));
}

void circb(int16_t x, int16_t y, int16_t r, uint8_t color) {
    DrawCircleLines(x, y, (float)r, tiny2d_get_palette(color));
}

void rect(int16_t x, int16_t y, int16_t w, int16_t h, uint8_t color) {
    if (w <= 0 || h <= 0) return;
    DrawRectangle(x, y, w, h, tiny2d_get_palette(color));
}

void rectb(int16_t x, int16_t y, int16_t w, int16_t h, uint8_t color) {
    if (w <= 0 || h <= 0) return;
    DrawRectangleLines(x, y, w, h, tiny2d_get_palette(color));
}

void elli(int16_t x, int16_t y, int16_t a, int16_t b, uint8_t color) {
    if (a <= 0 || b <= 0) return;
    DrawEllipse(x, y, (float)a, (float)b, tiny2d_get_palette(color));
}

void ellib(int16_t x, int16_t y, int16_t a, int16_t b, uint8_t color) {
    if (a <= 0 || b <= 0) return;
    DrawEllipseLines(x, y, (float)a, (float)b, tiny2d_get_palette(color));
}

// Рисует линию от (x0, y0) до (x1, y1)
void line(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint8_t color) {
    DrawLine(x0, y0, x1, y1, tiny2d_get_palette(color));
}

// Рисует пиксель в (x, y)
void pset(int16_t x, int16_t y, uint8_t color) {
    DrawPixel(x, y, tiny2d_get_palette(color));
}

// --- API для ввода ---
bool btn(uint8_t id)
{
    switch (id)
    {
        case 0: return IsKeyDown(KEY_A) || IsKeyDown(KEY_LEFT);
        case 1: return IsKeyDown(KEY_D) || IsKeyDown(KEY_RIGHT);
        case 2: return IsKeyDown(KEY_W) || IsKeyDown(KEY_UP);
        case 3: return IsKeyDown(KEY_S) || IsKeyDown(KEY_DOWN);
        case 4: return IsKeyDown(KEY_J) || IsKeyDown(KEY_Z);
        case 5: return IsKeyDown(KEY_K) || IsKeyDown(KEY_X);
        default: return false;
    }
}

bool btnp(uint8_t id)
{
    switch (id)
    {
        case 0: return IsKeyPressed(KEY_A) || IsKeyPressed(KEY_LEFT);
        case 1: return IsKeyPressed(KEY_D) || IsKeyPressed(KEY_RIGHT);
        case 2: return IsKeyPressed(KEY_W) || IsKeyPressed(KEY_UP);
        case 3: return IsKeyPressed(KEY_S) || IsKeyPressed(KEY_DOWN);
        case 4: return IsKeyPressed(KEY_J) || IsKeyPressed(KEY_Z);
        case 5: return IsKeyPressed(KEY_K) || IsKeyPressed(KEY_X);
        default: return false;
    }
}

// Мышь — перевод экранных координат в виртуальные
static tiny2d_mouse get_mouse_state(bool pressed) {
    tiny2d_mouse m = {0};

    // Экранные координаты
    Vector2 pos = GetMousePosition();

    // Масштаб и offset (как в главном цикле)
    int screenW = GetScreenWidth();
    int screenH = GetScreenHeight();

    float scaleX = (float)screenW / TINY2D_WIDTH;
    float scaleY = (float)screenH / TINY2D_HEIGHT;
    float scale  = (scaleX < scaleY) ? scaleX : scaleY;

    int offsetX = (int)((screenW - TINY2D_WIDTH  * scale) / 2);
    int offsetY = (int)((screenH - TINY2D_HEIGHT * scale) / 2);

    // Перевод в виртуальные координаты
    m.x = (int16_t)((pos.x - offsetX) / scale);
    m.y = (int16_t)((pos.y - offsetY) / scale);

    // Кнопки: down или pressed
    if (pressed) {
        m.left   = IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
        m.middle = IsMouseButtonPressed(MOUSE_BUTTON_MIDDLE);
        m.right  = IsMouseButtonPressed(MOUSE_BUTTON_RIGHT);
    } else {
        m.left   = IsMouseButtonDown(MOUSE_BUTTON_LEFT);
        m.middle = IsMouseButtonDown(MOUSE_BUTTON_MIDDLE);
        m.right  = IsMouseButtonDown(MOUSE_BUTTON_RIGHT);
    }

    // Прокрутка с клампом в int8_t
    Vector2 w = GetMouseWheelMoveV();
    float wx = w.x;
    float wy = w.y;

    if (wx >  32) wx =  32;
    if (wx < -32) wx = -32;
    if (wy >  32) wy =  32;
    if (wy < -32) wy = -32;

    m.scrollx = (int8_t)wx;
    m.scrolly = (int8_t)wy;

    return m;
}

tiny2d_mouse mouse(void) {
    return get_mouse_state(false);
}

tiny2d_mouse mousep(void) {
    return get_mouse_state(true);
}

// --- API для время ---
int64_t tiny2d_utime(void) {
    return (int64_t)time(NULL);
}


static double tiny2d_start_time = 0.0;

void tiny2d_time_reset(void) {
    tiny2d_start_time = GetTime();
}

double tiny2d_time(void) {
    return (GetTime() - tiny2d_start_time) * 1000.0; // миллисекунды с запуска
}