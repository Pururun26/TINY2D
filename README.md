# TINY2D

A tiny retro game engine for Lua. Built for making small 2D games fast.

- 160×160 virtual screen, 4× window
- 4-color palette (GB Chocolate) with runtime palette swapping
- Sprites from indexed PNG, transparency via magenta `#FF00FF`
- Sound and music loaded from files
- Tone synthesis — 4 channels (WASM-4 style)
- Mouse and keyboard input
- Save system — 64 slots × 8 bytes
- Cartridges — single `.tiny` file (ZIP with `data/` inside)
- Lua scripting with `require`
- Single ~1 MB executable, no external DLLs
- Windows, Linux, macOS and Web builds

---

## Font

Default font: `data/sprites/font.png` — a 128×128 sheet with 3×5 glyphs
(inspired by PICO-8). Replace it with your own if you want.

---

## Building

### Requirements

- **CMake 3.15+**
- **Compiler:** MinGW-w64 (Windows) / GCC (Linux) / Clang (macOS)
- **raylib 6.0** — download from [raylib releases](https://github.com/raysan5/raylib/releases)

### raylib folders

Place the matching raylib release in the project root:

| Platform | Folder | Archive |
|---|---|---|
| **Windows** | `raylib-6.0_win64_mingw-w64/` | `raylib-6.0_win64_mingw-w64.zip` |
| **Linux** | `raylib-6.0_linux_amd64/` | `raylib-6.0_linux_amd64.tar.gz` |
| **macOS** | `raylib-6.0_macos/` | `raylib-6.0_macos.tar.gz` |
| **Web** | `raylib-6.0_webassembly/` | `raylib-6.0_webassembly.zip` |

The Windows version is included in this repository.
Others must be downloaded separately.

### Desktop build (Windows / Linux / macOS)

```bash
cmake -B build
cmake --build build
```

Output:

- **Windows:** `build/tiny2d.exe`
- **Linux / macOS:** `build/tiny2d`

### Web build (Emscripten)

Requires [Emscripten SDK](https://emscripten.org).

```bash
source /path/to/emsdk/emsdk_env.sh
emcmake cmake -B build-web
cmake --build build-web
```

Output: `build-web/tiny2d.html`, `.js`, `.wasm`.

Then place `tiny2d.json` and your cart next to `tiny2d.html`, and serve over HTTP:

```bash
cd build-web
python -m http.server 8000
```

Open `http://localhost:8000/tiny2d.html`.

---

## Cartridges

A cartridge is a single `.tiny` file — a ZIP archive containing a `data/` folder:

```
game.tiny
└── data/
    ├── scripts/
    ├── sprites/
    ├── sounds/
    └── music/
```

### How the engine finds the game

When you run `tiny2d` without arguments, it searches in this order:

1. **`game.tiny`** next to the executable → extract to `_cart/`, run from there.
2. **`data/`** next to the executable → run in dev mode (no extraction).
3. **Nothing found** → engine exits with an error in the log.

You can also pass a cart explicitly:

```bash
tiny2d.exe mygame.tiny
tiny2d.exe other.tiny
```

This lets you ship one `tiny2d.exe` with multiple carts.

### Dev mode vs Cart mode

- **Dev mode** — run with a bare `data/` folder next to the exe. Edit `data/scripts/main.lua`, press **Ctrl+R** to reload. No repacking needed.
- **Cart mode** — run with a `.tiny` file. The engine extracts it to `_cart/`, runs, and cleans up on exit.

### Packing a cart

On Windows: right-click `src/data/` → *Send to → Compressed (zipped) folder*, rename to `game.tiny`.

On Linux/macOS:

```bash
cd src
zip -r ../game.tiny data/
```

### Web

For web builds, the cart is fetched by JavaScript and unpacked into virtual memory. Place next to `tiny2d.html`:

```
build-web/
├── tiny2d.html
├── tiny2d.js
├── tiny2d.wasm
├── tiny2d.json     ← { "cart": "game.tiny" }
└── game.tiny       ← the same cart
```

`tiny2d.json`:

```json
{
  "cart": "game.tiny"
}
```

---

## Running

**Ctrl+R** — reloads the game (Lua, sprites, font, sounds) without restarting the executable.

**F11** — toggle fullscreen.

---

## Architecture

```
tiny2d.exe
data/
├── scripts/
│   └── main.lua          ← game entry point
├── sprites/
│   ├── font.png          ← 128×128 font sheet (3×5 glyphs)
│   └── spritesheet.png   ← 256×256 sprites (8×8 each, 32×32 grid = 1024 sprites)
├── sounds/
│   ├── 0.wav             ← tiny2d.sfx(0)
│   ├── 1.wav             ← tiny2d.sfx(1)
│   └── ...               ← up to 63.wav (64 sounds total)
└── music/
    ├── 0.ogg             ← tiny2d.music(0)
    └── ...               ← up to 7.ogg (8 tracks total)
```

### `data/scripts/`

Lua game code. Entry point is always `main.lua`.

You can split your game into multiple files and use `require`:

```lua
-- main.lua
local player = require("player")
```

### `data/sprites/`

- **`font.png`** — 128×128 font sheet, glyphs 3×5 px.
- **`spritesheet.png`** — 256×256 spritesheet. Each sprite is 8×8 px.
  - Grid: 32×32 = **1024 sprites** total.
  - Sprite index `n` → column `n % 32`, row `n / 32`.
  - Transparent pixels: use magenta `#FF00FF`.
  - Colors: use the 4 palette colors only.

### `data/sounds/`

SFX files named `0.wav`, `1.wav`, ... `63.wav`. Loaded automatically at startup.

```lua
tiny2d.sfx(0)         -- play sound 0.wav at full volume
tiny2d.sfx(0, 0.5)    -- play at 50% volume
```

### `data/music/`

Music files named `0.ogg`, `1.ogg`, ... `7.ogg`. Loaded automatically.

```lua
tiny2d.music(0)         -- play 0.ogg in loop
tiny2d.music(0, 0.3)    -- play at 30% volume
tiny2d.music()          -- stop
```

---

## Lua API

**All API functions live in the `tiny2d` table.**

### Game callbacks

```lua
function tiny2d.init()     -- called once at startup (and after Ctrl+R reload)
end

function tiny2d.update()   -- logic, called 60 times per second
end

function tiny2d.draw()     -- rendering, called 60 times per second
end
```

All three are optional.

### Graphics

```lua
tiny2d.cls(color)                          -- clear screen
tiny2d.print(text, x, y, color)            -- draw text
tiny2d.fps(x, y, color)                    -- draw FPS counter
```

### Primitives

```lua
tiny2d.circ(x, y, r, color)                -- filled circle
tiny2d.circb(x, y, r, color)               -- circle border
tiny2d.elli(x, y, a, b, color)             -- filled ellipse
tiny2d.ellib(x, y, a, b, color)            -- ellipse border
tiny2d.rect(x, y, w, h, color)             -- filled rectangle
tiny2d.rectb(x, y, w, h, color)            -- rectangle border
tiny2d.line(x0, y0, x1, y1, color)         -- line
```

### Sprites

```lua
tiny2d.spr(n, x, y)                        -- draw 8×8 sprite n at (x, y)
tiny2d.spr(n, x, y, cols, rows)            -- draw a block of cols×rows sprites
tiny2d.spr(n, x, y, cols, rows, flip_x, flip_y)
```

`cols` and `rows` — how many 8×8 sprites wide and tall to draw. Default 1×1.

Example: `tiny2d.spr(0, 10, 10, 2, 3)` draws a 16×24 block (2 sprites wide, 3 tall) starting from sprite 0.

### Palette

```lua
tiny2d.pal()                -- reset to default
tiny2d.pal(m0, m1, m2, m3)  -- remap indices: new[i] = old[m[i]]
```

### Input — keyboard

```lua
tiny2d.btn(id)    -- is button held
tiny2d.btnp(id)   -- was button just pressed this frame
```

Button IDs:

| ID | Button |
|----|--------|
| 0  | LEFT   |
| 1  | RIGHT  |
| 2  | UP     |
| 3  | DOWN   |
| 4  | O (Z / J) |
| 5  | X (X / K) |

Keyboard mapping: arrows / WASD for direction, Z / J for `O`, X / K for `X`.

### Input — mouse

```lua
local m = tiny2d.mouse()   -- current state (buttons held)
local m = tiny2d.mousep()  -- state at this frame (buttons just pressed)
```

Both return a table:

| Field | Type | Meaning |
|-------|------|---------|
| `m.x` | integer | cursor X in virtual screen coords (0–159) |
| `m.y` | integer | cursor Y in virtual screen coords (0–159) |
| `m.left` | bool | left button |
| `m.middle` | bool | middle button |
| `m.right` | bool | right button |
| `m.scrollx` | integer | horizontal scroll delta (−31..32) |
| `m.scrolly` | integer | vertical scroll delta (−31..32) |

Example:

```lua
function tiny2d.draw()
    tiny2d.cls(0)
    local m = tiny2d.mouse()
    tiny2d.circ(m.x, m.y, 2, 3)
    if m.left then tiny2d.rectb(m.x - 4, m.y - 4, 8, 8, 1) end
end
```

### Time

```lua
tiny2d.utime()    -- Unix timestamp in seconds (integer)
tiny2d.time()     -- milliseconds elapsed since game start (float)
```

- `tiny2d.utime()` — for idle games, persistent progress, saving timestamps.
- `tiny2d.time()` — for animations, timers, blinking.

```lua
-- animation
local elapsed = tiny2d.time() / 1000
tiny2d.circ(80 + math.sin(elapsed * 2) * 30, 80, 5, 3)

-- idle
local last = tiny2d.load(0)
local now = tiny2d.utime()
coins = coins + (now - last) * 0.1
tiny2d.save(0, now)
```

### Sound

```lua
tiny2d.sfx(n)             -- play sound n
tiny2d.sfx(n, volume)     -- play with volume 0.0–1.0
tiny2d.music(n)           -- play music n in loop
tiny2d.music(n, volume)   -- play with volume
tiny2d.music()            -- stop current music
```

### Tone synthesis

Procedural sound generation — like WASM-4. Useful for small carts,
no WAV files needed.

```lua
tiny2d.tone(frequency, duration, volume, flags)
```

- `frequency` — Hz (e.g. 440, 880)
- `duration` — in frames (60 frames = 1 second)
- `volume` — 0 to 100
- `flags` — channel / waveform (0–3)

4 channels, one sound per channel. A new tone on the same channel
replaces the previous one.

| flags | Channel | Waveform |
|-------|---------|----------|
| 0 | 0 | pulse 12.5% |
| 1 | 1 | pulse 25% |
| 2 | 2 | triangle |
| 3 | 3 | noise |

Examples:

```lua
-- Jump — pulse 12.5%
tiny2d.tone(660, 6, 80, 0)

-- Shoot — pulse 25%
tiny2d.tone(880, 4, 70, 1)

-- Bassy note — triangle
tiny2d.tone(220, 30, 40, 2)

-- Explosion — noise
tiny2d.tone(0, 15, 90, 3)

-- All 4 can play at the same time
```

### Save

```lua
tiny2d.save(pos, value)   -- write value (int64) to slot pos (0–63)
tiny2d.load(pos)          -- read value from slot pos
```

- On desktop: auto-saved to `save/storage.bin` on window close and every 10 minutes.
- On web: saved to browser `localStorage` immediately on every `tiny2d.save()`.

---

## Lua modules

TINY2D comes with two optional Lua modules:

### `camera.lua` — scrolling camera

Adds `tiny2d.camera(x, y)` to the API. When called, all subsequent
drawing functions are offset by `(-x, -y)`. Call `tiny2d.camera()`
to reset.

```lua
require("camera")

function tiny2d.draw()
    tiny2d.cls(3)
    tiny2d.camera(player.x - 76, player.y - 76)
    tiny2d.spr(0, player.x, player.y)
    tiny2d.camera()   -- reset for HUD
    tiny2d.print("HP", 1, 1, 0)
end
```

### `map.lua` — tilemap drawing

Adds `tiny2d.map()`, `tiny2d.mget()`, `tiny2d.mset()` to the API.
Draws a flat tilemap loaded from Tiled.

```lua
require("map")

function tiny2d.init()
    tiny2d.map_data = { 2, 2, 0, 0, 2, 2, ... }
    tiny2d.map_w = 128
    tiny2d.map_h = 128
end

function tiny2d.draw()
    tiny2d.map(0, 0, 0, 0, 20, 20)
end
```

Fields:
- `tiny2d.map_data` — flat array from Tiled (1-based GIDs, 0 = empty)
- `tiny2d.map_w`, `tiny2d.map_h` — map dimensions in tiles (defaults 128×128)

Functions:
- `tiny2d.map(celx, cely, sx, sy, celw, celh)` — draw a region
- `tiny2d.mget(x, y)` — read tile (0-based sprite id, 0 = empty)
- `tiny2d.mset(x, y, id)` — write tile

Convention: **sprite 0 is reserved** — don't use it in maps.

---

## Example

`data/scripts/main.lua`:

```lua
local x, y

function tiny2d.init()
    x = 80
    y = 80
end

function tiny2d.update()
    if tiny2d.btn(0) then x = x - 1 end
    if tiny2d.btn(1) then x = x + 1 end
    if tiny2d.btn(2) then y = y - 1 end
    if tiny2d.btn(3) then y = y + 1 end

    if tiny2d.btnp(4) then tiny2d.sfx(0) end
end

function tiny2d.draw()
    tiny2d.cls(0)
    tiny2d.spr(0, x - 4, y - 4)
    tiny2d.print("HELLO", 4, 4, 3)
end
```

---

## Migration from 1.x

**TINY2D 2.0.0 is not backwards compatible.**

In 1.x, API functions were global (`spr`, `cls`, `btn`, ...).
In 2.x, they live in the `tiny2d` table.

```lua
-- 1.x
function update()
    cls(0)
    spr(0, 10, 10)
end

-- 2.x
function tiny2d.update()
    tiny2d.cls(0)
    tiny2d.spr(0, 10, 10)
end
```

Game callbacks (`init`, `update`, `draw`) must also be `tiny2d.init`, `tiny2d.update`, `tiny2d.draw`.

---

## Credits

- [raylib](https://raylib.com) — zlib license
- [Lua 5.4.9](https://lua.org) — MIT license
- GB Chocolate palette by [GrafxKid](https://lospec.com/palette-list/gb-chocolate)

---

## License

MIT — see `LICENSE`.