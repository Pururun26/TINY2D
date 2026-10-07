cmake_minimum_required(VERSION 3.15)
project(tiny2d C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# ============================================================
# Пути к raylib
# ============================================================
set(RAYLIB_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/raylib/include")
set(RAYLIB_LIB_DIR     "${CMAKE_SOURCE_DIR}/raylib/lib")
set(RAYLIB_WEB_DIR     "${CMAKE_SOURCE_DIR}/raylib-6.0_webassembly")

# ============================================================
# Lua 5.4.9 — общий для всех платформ
# ============================================================
set(LUA_DIR "${CMAKE_SOURCE_DIR}/lua")

set(LUA_SRC
    ${LUA_DIR}/lapi.c
    ${LUA_DIR}/lauxlib.c
    ${LUA_DIR}/lbaselib.c
    ${LUA_DIR}/lcode.c
    ${LUA_DIR}/lcorolib.c
    ${LUA_DIR}/lctype.c
    ${LUA_DIR}/ldblib.c
    ${LUA_DIR}/ldebug.c
    ${LUA_DIR}/ldo.c
    ${LUA_DIR}/ldump.c
    ${LUA_DIR}/lfunc.c
    ${LUA_DIR}/lgc.c
    ${LUA_DIR}/linit.c
    ${LUA_DIR}/liolib.c
    ${LUA_DIR}/llex.c
    ${LUA_DIR}/lmathlib.c
    ${LUA_DIR}/lmem.c
    ${LUA_DIR}/loadlib.c
    ${LUA_DIR}/lobject.c
    ${LUA_DIR}/lopcodes.c
    ${LUA_DIR}/loslib.c
    ${LUA_DIR}/lparser.c
    ${LUA_DIR}/lstate.c
    ${LUA_DIR}/lstring.c
    ${LUA_DIR}/lstrlib.c
    ${LUA_DIR}/ltable.c
    ${LUA_DIR}/ltablib.c
    ${LUA_DIR}/ltm.c
    ${LUA_DIR}/lundump.c
    ${LUA_DIR}/lutf8lib.c
    ${LUA_DIR}/lvm.c
    ${LUA_DIR}/lzio.c
)

add_library(lua STATIC ${LUA_SRC})
target_include_directories(lua PUBLIC "${LUA_DIR}")

if(NOT WIN32)
    find_package(Threads REQUIRED)
    target_link_libraries(lua PUBLIC m ${CMAKE_DL_LIBS})
endif()

# ============================================================
# tiny2d
# ============================================================
set(SOURCES
    src/tiny2d.c
    src/tiny2d_api.c
    src/tiny2d_api_lua.c
    src/tiny2d_log.c
    src/tiny2d_sprites.c
    src/tiny2d_audio.c
    src/tiny2d_save.c
)

add_executable(tiny2d ${SOURCES})

target_include_directories(tiny2d PRIVATE
    "${CMAKE_SOURCE_DIR}/src"
)

# ============================================================
# Emscripten (web) — сборка через emcmake
# ============================================================
if(EMSCRIPTEN)

    target_include_directories(tiny2d PRIVATE
        "${RAYLIB_WEB_DIR}/include"
    )

    target_link_directories(tiny2d PRIVATE
        "${RAYLIB_WEB_DIR}/lib"
    )

    # raylib для web называется libraylib.web.a
    target_link_libraries(tiny2d PRIVATE raylib.web lua)

    target_link_options(tiny2d PRIVATE
        -sUSE_GLFW=3
        -sASYNCIFY
        -sTOTAL_MEMORY=64MB
        -sALLOW_MEMORY_GROWTH=1
        -sSTACK_SIZE=1MB
        "-sEXPORTED_RUNTIME_METHODS=['callMain','FS']"
        "--shell-file=${CMAKE_SOURCE_DIR}/web/shell.html"
        -O2
    )

    set_target_properties(tiny2d PROPERTIES
        SUFFIX ".html"
        OUTPUT_NAME "tiny2d"
        WIN32_EXECUTABLE FALSE
    )

# ============================================================
# Windows / Linux / macOS — обычная сборка
# ============================================================
else()

    # Оптимизация под размер (Release)
    if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
        set(CMAKE_C_FLAGS_RELEASE "-Os -DNDEBUG")
        target_compile_options(tiny2d PRIVATE
            -ffunction-sections
            -fdata-sections
        )
    endif()

    # Скрыть окно консоли на Windows (GUI-приложение)
    set_target_properties(tiny2d PROPERTIES WIN32_EXECUTABLE TRUE)

    if(WIN32 AND CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
        target_link_options(tiny2d PRIVATE
            -mwindows
            -Wl,--gc-sections
            -s
        )
    elseif(APPLE)
        target_link_options(tiny2d PRIVATE
            -Wl,-dead_strip
        )
    else()
        target_link_options(tiny2d PRIVATE
            -Wl,--gc-sections
            -s
        )
    endif()

    target_include_directories(tiny2d PRIVATE
        "${RAYLIB_INCLUDE_DIR}"
    )

    target_link_directories(tiny2d PRIVATE "${RAYLIB_LIB_DIR}")

    # Платформенные библиотеки
    if(WIN32)
        target_link_libraries(tiny2d PRIVATE raylib lua gdi32 winmm)
    elseif(APPLE)
        find_library(COCOA_LIBRARY Cocoa REQUIRED)
        find_library(IOKIT_LIBRARY IOKit REQUIRED)
        find_library(COREVIDEO_LIBRARY CoreVideo REQUIRED)
        find_library(OPENGL_LIBRARY OpenGL REQUIRED)
        target_link_libraries(tiny2d PRIVATE raylib lua
            ${COCOA_LIBRARY} ${IOKIT_LIBRARY}
            ${COREVIDEO_LIBRARY} ${OPENGL_LIBRARY})
    else()
        find_package(Threads REQUIRED)
        target_link_libraries(tiny2d PRIVATE raylib lua
            m ${CMAKE_DL_LIBS} Threads::Threads)
    endif()

    # Копирование data рядом с exe (если есть)
    if(EXISTS "${CMAKE_SOURCE_DIR}/src/data")
        add_custom_command(TARGET tiny2d POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_directory
                "${CMAKE_SOURCE_DIR}/src/data"
                "$<TARGET_FILE_DIR:tiny2d>/data"
            COMMENT "Copying data to build directory"
        )
    else()
        message(STATUS "src/data/ not found — skipping copy")
    endif()

    # ============================================================
    # Запуск (только для нативной сборки)
    # ============================================================
    add_custom_target(run
        COMMAND $<TARGET_FILE:tiny2d>
        DEPENDS tiny2d
        WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
    )

endif()