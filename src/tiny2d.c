#include "tiny2d_api.h"
#include "tiny2d_api_lua.h"
#include "tiny2d_conf.h"
#include "tiny2d_log.h"
#include "tiny2d_sprites.h"
#include "tiny2d_audio.h"
#include "tiny2d_save.h"
#include "raylib.h"
#include <stdio.h>
#include <string.h>

static RenderTexture2D target;
static lua_State *L = NULL;

// ============================================================
// Вызов Lua-функций из C
// ============================================================

static void call_lua(const char *name) {
    if (!L) return;
    lua_getglobal(L, name);
    if (!lua_isfunction(L, -1)) { lua_pop(L, 1); return; }
    if (lua_pcall(L, 0, 0, 0) != LUA_OK) {
        tiny2d_log("lua error in %s: %s", name, lua_tostring(L, -1));
        lua_pop(L, 1);
    }
}

// ============================================================
// Настройка package.path по папке игры
// ============================================================

static void setup_lua_path(lua_State *Lstate, const char *game_path) {
    const char *slash  = strrchr(game_path, '/');
    const char *bslash = strrchr(game_path, '\\');
    if (bslash && (!slash || bslash > slash)) slash = bslash;

    if (!slash) return;   // игра в текущей папке — package.path уже рабочий

    int dir_len = (int)(slash - game_path);
    char lua_path[1024];
    snprintf(lua_path, sizeof(lua_path),
             "%.*s/?.lua;%.*s/?/init.lua;./?.lua",
             dir_len, game_path,
             dir_len, game_path);

    lua_getglobal(Lstate, "package");
    lua_pushstring(Lstate, lua_path);
    lua_setfield(Lstate, -2, "path");
    lua_pop(Lstate, 1);
}

// ============================================================
// Перезагрузка игры (Ctrl+R)
//   Перезагружает: Lua, спрайты, шрифт, звуки.
//   НЕ трогает: окно, storage.bin, рендер-текстуру.
// ============================================================

static void reload_game(const char *game_path) {
    tiny2d_log("TINY2D: Reloading...");

    // 1. Закрыть старый Lua, если был
    if (L) {
        lua_close(L);
        L = NULL;
    }

    // 2. Перезагрузить спрайты
    {
        char path[1024];
        snprintf(path, sizeof(path),
                 "%s%s", GetApplicationDirectory(), TINY2D_SPRITESHEET_PATH);
        tiny2d_load_spritesheet(path);
    }

    // 3. Перезагрузить шрифт
    {
        char path[1024];
        snprintf(path, sizeof(path),
                 "%s%s", GetApplicationDirectory(), TINY2D_FONT_PATH);
        tiny2d_load_font(path);
    }

    // 4. Перезагрузить звуки и музыку
    tiny2d_audio_close();
    tiny2d_audio_init();

    tiny2d_time_reset();

    // 5. Создать новый Lua
    L = luaL_newstate();

    // Открываем только безопасные библиотеки
    luaL_requiref(L, "_G",        luaopen_base,      1); lua_pop(L, 1);
    luaL_requiref(L, "math",      luaopen_math,      1); lua_pop(L, 1);
    luaL_requiref(L, "string",    luaopen_string,    1); lua_pop(L, 1);
    luaL_requiref(L, "table",     luaopen_table,     1); lua_pop(L, 1);
    luaL_requiref(L, "coroutine", luaopen_coroutine, 1); lua_pop(L, 1);
    luaL_requiref(L, "utf8",      luaopen_utf8,      1); lua_pop(L, 1);
    luaL_requiref(L, "package",   luaopen_package,   1); lua_pop(L, 1);
    // НЕ открываем: io, os, debug
    tiny2d_register_api(L);
    setup_lua_path(L, game_path);

    if (luaL_dofile(L, game_path) != LUA_OK) {
        tiny2d_log("lua error: %s", lua_tostring(L, -1));
        lua_close(L);
        L = NULL;
        // Игра не запущена, но окно живо — можно править и жать Ctrl+R снова
    } else {
        call_lua("init");
        tiny2d_log("TINY2D: Reloaded.");
    }
}

// ============================================================
// main
// ============================================================

int main(int argc, char **argv) {
    // ============================================================
    // 1. Логирование
    // ============================================================
    tiny2d_log_init();

    // ============================================================
    // 2. Путь к main.lua
    // ============================================================
    char game_path[1024];
    if (argc >= 2) {
        snprintf(game_path, sizeof(game_path), "%s", argv[1]);
    } else {
        snprintf(game_path, sizeof(game_path),
                 "%s%s", GetApplicationDirectory(), TINY2D_MAIN_SCRIPT);
    }
    tiny2d_log("game path: %s", game_path);

    // ============================================================
    // 3. Raylib
    // ============================================================
    InitWindow(TINY2D_WINDOW_WIDTH, TINY2D_WINDOW_HEIGHT, TINY2D_TITLE);

    // Скрываем системный курсор
    HideCursor();
    
    SetTargetFPS(TINY2D_FPS);

    target = LoadRenderTexture(TINY2D_WIDTH, TINY2D_HEIGHT);
    SetTextureFilter(target.texture, TEXTURE_FILTER_POINT);

    // Сохранение — грузим один раз
    tiny2d_save_init();

    // ============================================================
    // 4. Первая загрузка игры (Lua + спрайты + шрифт + звуки)
    // ============================================================
    reload_game(game_path);

    if (!L) {
        // main.lua с ошибкой — окно остаётся, можно править и жать Ctrl+R
        tiny2d_log("TINY2D: Game failed to load. Press Ctrl+R to reload.");
    }

    int autosave_counter = 0;
    const int AUTOSAVE_INTERVAL = 60 * 10 * 60;   // 10 минут при 60 FPS

    // ============================================================
    // 5. Главный цикл
    // ============================================================
    while (!WindowShouldClose()) {
        // Ctrl+R — перезагрузка игры
        if (IsKeyDown(KEY_LEFT_CONTROL) && IsKeyPressed(KEY_R)) {
            reload_game(game_path);
        }

        // Музыка стримится каждый кадр
        music_update();

        if (IsKeyPressed(KEY_F11)) ToggleFullscreen();

        autosave_counter++;
        if (autosave_counter >= AUTOSAVE_INTERVAL) {
            tiny2d_save_all();
            autosave_counter = 0;
        }

        call_lua("update");

        BeginTextureMode(target);
            ClearBackground(BLACK);
            call_lua("draw");
        EndTextureMode();

        BeginDrawing();
            ClearBackground(BLACK);
            int screenW = GetScreenWidth();
            int screenH = GetScreenHeight();

            float scaleX = (float)screenW / TINY2D_WIDTH;
            float scaleY = (float)screenH / TINY2D_HEIGHT;
            float current_scale = (scaleX < scaleY) ? scaleX : scaleY;

            int offsetX = (int)((screenW - TINY2D_WIDTH  * current_scale) / 2);
            int offsetY = (int)((screenH - TINY2D_HEIGHT * current_scale) / 2);

            DrawTexturePro(
                target.texture,
                (Rectangle){ 0, 0, (float)TINY2D_WIDTH, (float)-TINY2D_HEIGHT },
                (Rectangle){ (float)offsetX, (float)offsetY,
                             TINY2D_WIDTH  * current_scale,
                             TINY2D_HEIGHT * current_scale },
                (Vector2){ 0, 0 },
                0.0f,
                WHITE
            );
        EndDrawing();
    }

    // ============================================================
    // 6. Завершение
    // ============================================================
    tiny2d_log("closing");

    tiny2d_save_all();
    tiny2d_audio_close();
    UnloadRenderTexture(target);
    CloseWindow();

    if (L) lua_close(L);

    tiny2d_log_close();
    return 0;
}