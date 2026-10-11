#include "tiny2d_api.h"
#include "tiny2d_api_lua.h"
#include "tiny2d_conf.h"
#include "tiny2d_log.h"
#include "tiny2d_sprites.h"
#include "tiny2d_audio.h"
#include "tiny2d_save.h"

#ifndef __EMSCRIPTEN__
#include "tiny2d_cart.h"
#endif

#include "raylib.h"
#include <stdio.h>
#include <string.h>

static RenderTexture2D target;
static lua_State *L = NULL;

char g_data_dir[1024] = {0};

// ============================================================
// Вызов Lua-функций из C
// ============================================================

static void call_lua(const char *name) {
    if (!L) return;

    lua_getglobal(L, "tiny2d");
    if (!lua_istable(L, -1)) {
        lua_pop(L, 1);
        return;
    }

    lua_getfield(L, -1, name);
    lua_remove(L, -2);   // убрать tiny2d со стека

    if (!lua_isfunction(L, -1)) {
        lua_pop(L, 1);
        return;
    }

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
                 "%s%s", g_data_dir, TINY2D_SPRITESHEET_PATH);
        tiny2d_load_spritesheet(path);
    }

    // 3. Перезагрузить шрифт
    {
        char path[1024];
        snprintf(path, sizeof(path),
                 "%s%s", g_data_dir, TINY2D_FONT_PATH);
        tiny2d_load_font(path);
    }

    // 4. Перезагрузить звуки и музыку
    tiny2d_audio_close();
    tiny2d_audio_init(g_data_dir);

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

    const char *exe_dir = GetApplicationDirectory();

    char game_path[1024]     = {0};
    char cart_out[1024]      = {0};
    char cart_path[1024]     = {0};

    // ============================================================
    // 2. Определяем источник игры
    //    1) аргумент:   tiny2d.exe shmup.tiny
    //    2) game.tiny   рядом с exe
    //    3) data/       рядом с exe (режим разработки)
    // ============================================================

    if (argc >= 2) {
        snprintf(cart_path, sizeof(cart_path), "%s", argv[1]);
        tiny2d_log("TINY2D: cart from argument: %s", cart_path);
    } else {
        char candidate[1024];
        snprintf(candidate, sizeof(candidate), "%sgame.tiny", exe_dir);
        if (FileExists(candidate)) {
            snprintf(cart_path, sizeof(cart_path), "%s", candidate);
            tiny2d_log("TINY2D: found game.tiny");
        }
    }

        // ============================================================
    // 2. Определяем источник игры
    //    Desktop:  1) аргумент .tiny  2) game.tiny  3) data/
    //    Web:      JS уже распаковал в /data/, идём сразу в dev
    // ============================================================

#ifndef __EMSCRIPTEN__
    if (argc >= 2) {
        snprintf(cart_path, sizeof(cart_path), "%s", argv[1]);
        tiny2d_log("TINY2D: cart from argument: %s", cart_path);
    } else {
        char candidate[1024];
        snprintf(candidate, sizeof(candidate), "%sgame.tiny", exe_dir);
        if (FileExists(candidate)) {
            snprintf(cart_path, sizeof(cart_path), "%s", candidate);
            tiny2d_log("TINY2D: found game.tiny");
        }
    }

    if (cart_path[0]) {
        // ─── Картридж найден — распаковываем в _cart/ ───
        snprintf(cart_out, sizeof(cart_out), "%s_cart", exe_dir);

        if (!tiny2d_cart_extract(cart_path, cart_out)) {
            tiny2d_log("TINY2D: failed to extract '%s'", cart_path);
            tiny2d_log_close();
            return 1;
        }

        snprintf(game_path, sizeof(game_path),
                 "%s/data/scripts/main.lua", cart_out);
        snprintf(g_data_dir, sizeof(g_data_dir), "%s/", cart_out);
    } else
#endif
    {
        // ─── Режим разработки — data/ рядом с exe ───
        char data_dir[1024];
        snprintf(data_dir, sizeof(data_dir), "%sdata", exe_dir);

        if (!DirectoryExists(data_dir)) {
            tiny2d_log("TINY2D: no game.tiny and no data/ found.");
            tiny2d_log_close();
            return 1;
        }

        snprintf(game_path, sizeof(game_path),
                 "%s%s", exe_dir, TINY2D_MAIN_SCRIPT);
        snprintf(g_data_dir, sizeof(g_data_dir), "%s", exe_dir);
        tiny2d_log("TINY2D: using data/ directly (dev mode)");
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
    
#ifndef __EMSCRIPTEN__
    // Удаляем распакованный картридж
    if (cart_out[0]) {
        tiny2d_cart_remove(cart_out);
    }
#endif

    CloseWindow();

    if (L) lua_close(L);

    tiny2d_log_close();
    return 0;
}