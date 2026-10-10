#include "tiny2d_sprites.h"
#include "tiny2d_conf.h"
#include "tiny2d_api.h"
#include "raylib.h"

#include <stdlib.h>

// ============================================================
// Состояние
// ============================================================

static uint8_t  *sprite_indices = NULL;   // RGBA → индексы палитры (0–3, 255 = прозрачный)
static Texture2D sprite_sheet   = {0};    // текстура, где R=G=B=индекс, A=прозрачность
static bool      loaded         = false;

// Шейдер палитры: берёт индекс из текстуры и подставляет цвет из uniform.
static Shader palette_shader = {0};
static int palette_locs[4] = { -1, -1, -1, -1 };  // location uniform "palette[4]"
static bool   shader_ready   = false;
static bool   palette_dirty  = true;      // нужно ли обновить uniform палитры

// ============================================================
// Исходник шейдера (GLSL 330)
// ============================================================

static const char *PALETTE_FS =
    "varying vec2 fragTexCoord;\n"
    "varying vec4 fragColor;\n"
    "uniform sampler2D texture0;\n"
    "uniform vec4 colDiffuse;\n"
    "uniform vec4 pal0;\n"
    "uniform vec4 pal1;\n"
    "uniform vec4 pal2;\n"
    "uniform vec4 pal3;\n"
    "void main()\n"
    "{\n"
    "    vec4 texel = texture2D(texture0, fragTexCoord);\n"
    "    if (texel.a < 0.5) discard;\n"
    "    float idx = texel.r * 255.0;\n"
    "    vec4 col;\n"
    "    if (idx < 0.5) col = pal0;\n"
    "    else if (idx < 1.5) col = pal1;\n"
    "    else if (idx < 2.5) col = pal2;\n"
    "    else col = pal3;\n"
    "    gl_FragColor = col * colDiffuse;\n"
    "}\n";

// ============================================================
// Конвертация цвета в индекс палитры
// ============================================================

// Возвращает ближайший индекс палитры для цвета c.
// Прозрачные пиксели → 255 (спец-значение «не рисовать»).
static uint8_t nearest_palette_index(Color c) {
    // прозрачные пиксели (альфа = 0)
    if (c.a == 0) return 255;

    // маджента (#FF00FF) — ключ прозрачности
    if (c.r == 255 && c.g == 0 && c.b == 255) return 255;

    // иначе — ближайший цвет палитры
    int best = 0;
    int best_dist = 1 << 30;
    for (int i = 0; i < TINY2D_COLOR_COUNT; i++) {
        Color p = tiny2d_get_palette(i);
        int dr = (int)c.r - p.r;
        int dg = (int)c.g - p.g;
        int db = (int)c.b - p.b;
        int dist = dr*dr + dg*dg + db*db;
        if (dist < best_dist) {
            best_dist = dist;
            best = i;
        }
    }
    return (uint8_t)best;
}

// ============================================================
// Uniform палитры — заливаем цвета в шейдер
// ============================================================

static void update_palette_uniform(void) {
    if (!shader_ready || !palette_dirty) return;

    // 4 цвета по 4 компонента (RGBA) = 16 float
    float data[16];
    for (int i = 0; i < TINY2D_COLOR_COUNT; i++) {
        Color c = tiny2d_get_palette(i);
        data[i*4 + 0] = c.r / 255.0f;
        data[i*4 + 1] = c.g / 255.0f;
        data[i*4 + 2] = c.b / 255.0f;
        data[i*4 + 3] = c.a / 255.0f;
    }

    for (int i = 0; i < 4; i++) {
        if (palette_locs[i] >= 0) {
            SetShaderValue(palette_shader, palette_locs[i],
                        &data[i * 4], SHADER_UNIFORM_VEC4);
        }
    }
    palette_dirty = false;
}

// ============================================================
// Загрузка
// ============================================================

void tiny2d_load_spritesheet(const char *filepath) {
    Image img = LoadImage(filepath);

    if (img.data == NULL) {
        // Заглушка: пустой лист 256×256, все пиксели прозрачные.
        img = GenImageColor(TINY2D_SPRITE_SHEET_W,
                            TINY2D_SPRITE_SHEET_H, BLANK);
        TraceLog(LOG_WARNING,
                 "TINY2D: Spritesheet '%s' not found. Using blank.", filepath);
    }

    if (img.format != PIXELFORMAT_UNCOMPRESSED_R8G8B8A8) {
        ImageFormat(&img, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);
    }

    int count = img.width * img.height;

    // Освобождаем прошлый буфер, если был
    if (sprite_indices) free(sprite_indices);
    sprite_indices = malloc(count);
    if (!sprite_indices) {
        UnloadImage(img);
        return;
    }

    // RGBA → индексы палитры
    Color *px = (Color *)img.data;
    for (int i = 0; i < count; i++) {
        sprite_indices[i] = nearest_palette_index(px[i]);
    }

    // Готовим ИНДЕКСНУЮ текстуру: R=G=B=индекс, A=0 для прозрачного.
    // Шейдер потом превратит эти индексы в цвета палитры.
    Color *indexed = malloc(count * sizeof(Color));
    if (!indexed) {
        UnloadImage(img);
        return;
    }
    for (int i = 0; i < count; i++) {
        uint8_t idx = sprite_indices[i];
        if (idx == 255) {
            indexed[i] = (Color){ 0, 0, 0, 0 };      // прозрачный
        } else {
            indexed[i] = (Color){ idx, idx, idx, 255 };
        }
    }

    Image idx_img = {
        .data    = indexed,
        .width   = img.width,
        .height  = img.height,
        .mipmaps = 1,
        .format  = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8
    };

    UnloadImage(img);

    // Создаём текстуру из индексов
    if (sprite_sheet.id != 0) UnloadTexture(sprite_sheet);
    sprite_sheet = LoadTextureFromImage(idx_img);
    SetTextureFilter(sprite_sheet, TEXTURE_FILTER_POINT);

    free(indexed);

    // Загружаем шейдер палитры — один раз
    if (!shader_ready) {
        palette_shader = LoadShaderFromMemory(0, PALETTE_FS);
        if (palette_shader.id == 0) {
            TraceLog(LOG_ERROR, "TINY2D: Palette shader failed to load!");
        } else {
            palette_locs[0] = GetShaderLocation(palette_shader, "pal0");
            palette_locs[1] = GetShaderLocation(palette_shader, "pal1");
            palette_locs[2] = GetShaderLocation(palette_shader, "pal2");
            palette_locs[3] = GetShaderLocation(palette_shader, "pal3");

            TraceLog(LOG_INFO, "TINY2D: pal0 loc=%d, pal1 loc=%d, pal2 loc=%d, pal3 loc=%d",
                    palette_locs[0], palette_locs[1], palette_locs[2], palette_locs[3]);

            if (palette_locs[0] < 0 || palette_locs[1] < 0 ||
                palette_locs[2] < 0 || palette_locs[3] < 0) {
                TraceLog(LOG_WARNING, "TINY2D: Some palette uniforms not found!");
            }
            shader_ready = true;
        }
    }

    palette_dirty = true;
    loaded        = true;

    TraceLog(LOG_INFO, "TINY2D: Spritesheet '%s' loaded (%dx%d).",
             filepath, idx_img.width, idx_img.height);
}

// ============================================================
// Rebuild — теперь просто помечаем палитру «грязной».
// Реальное обновление произойдёт перед первым spr за кадр.
// ============================================================

void tiny2d_rebuild_spritesheet(void) {
    palette_dirty = true;
}

// ============================================================
// Рисование
// ============================================================

void spr_pro(int16_t n, int16_t x, int16_t y,
             uint8_t w, uint8_t h,
             bool flip_x, bool flip_y) {
    if (!loaded) return;
    if (w == 0 || h == 0) return;
    if (n < 0) return;

    int16_t col = n % TINY2D_SPRITE_SHEET_TILES;
    int16_t row = n / TINY2D_SPRITE_SHEET_TILES;

    int16_t pixel_w = w * TINY2D_SPRITE_SIZE;
    int16_t pixel_h = h * TINY2D_SPRITE_SIZE;

    Rectangle src = {
        (float)(col * TINY2D_SPRITE_SIZE),
        (float)(row * TINY2D_SPRITE_SIZE),
        (float)pixel_w,
        (float)pixel_h
    };

    if (flip_x) src.width  = -(float)pixel_w;
    if (flip_y) src.height = -(float)pixel_h;

    Rectangle dest = { (float)x, (float)y, (float)pixel_w, (float)pixel_h };

    if (shader_ready) {
        update_palette_uniform();
        BeginShaderMode(palette_shader);
            DrawTexturePro(sprite_sheet, src, dest, (Vector2){0, 0}, 0.0f, WHITE);
        EndShaderMode();
    } else {
        // Fallback — без палитры, увидим серые оттенки индексов
        DrawTexturePro(sprite_sheet, src, dest, (Vector2){0, 0}, 0.0f, WHITE);
    }
}

void spr(int16_t n, int16_t x, int16_t y) {
    spr_pro(n, x, y, 1, 1, false, false);
}