# TINY2D

A tiny retro game engine for Lua. Built for making small 2D games fast.

- 160×160 virtual screen, 4× window
- 4-color palette (GB Chocolate) with runtime palette swapping
- Sprites from indexed PNG, transparency via magenta `#FF00FF`
- Sound and music loaded from files
- Mouse and keyboard input
- Save system — 64 slots × 8 bytes
- Lua scripting with `require`
- Single ~1 MB executable, no external DLLs
- Web build via Emscripten

---

## Font

Default font: `data/sprites/font.png` — a 128×128 sheet with 3×5 glyphs
(inspired by PICO-8). Replace it with your own if you want.

---

## Building

Requirements:
- CMake 3.15+
- MinGW-w64 (Windows) / GCC (Linux) / Clang (macOS)

```bash
cmake -B build
cmake --build build
```

The executable will be at `build/tiny2d.exe`.

### Web build

Requires [Emscripten SDK](https://emscripten.org).

```bash
emcmake cmake -B build-web
cmake --build build-web
```

Output: `build-web/tiny2d.html`, `.js`, `.wasm`.

To run, place a `data.zip` (your `data/` folder zipped) next to `tiny2d.html`,
then serve over HTTP:

```bash
cd build-web
python -m http.server 8000
```

Open `http://localhost:8000/tiny2d.html`.

---

## Running

`tiny2d.exe` expects a `data/` folder **next to it**. If `data/` is missing, the engine has nothing to run.

To start, download the release `.zip` (which includes `data/`) or create your own `data/` folder — see the structure below.

**Ctrl+R** — reloads the game (Lua, sprites, font, sounds) without restarting the executable.

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
│   ├── 0.wav             ← sfx(0)
│   ├── 1.wav             ← sfx(1)
│   └── ...               ← up to 63.wav (64 sounds total)
└── music/
    ├── 0.ogg             ← music(0)
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
sfx(0)         -- play sound 0.wav at full volume
sfx(0, 0.5)    -- play at 50% volume
```

### `data/music/`

Music files named `0.ogg`, `1.ogg`, ... `7.ogg`. Loaded automatically.

```lua
music(0)         -- play 0.ogg in loop
music(0, 0.3)    -- play at 30% volume
music()          -- stop
```

---

## Lua API

### Game callbacks

```lua
function init()     -- called once at startup (and after Ctrl+R reload)
end

function update()   -- logic, called 60 times per second
end

function draw()     -- rendering, called 60 times per second
end
```

All three are optional.

### Graphics

```lua
cls(color)                          -- clear screen
print(text, x, y, color)            -- draw text
fps(x, y, color)                    -- draw FPS counter
```

### Primitives

```lua
circ(x, y, r, color)                -- filled circle
circb(x, y, r, color)               -- circle border
elli(x, y, a, b, color)             -- filled ellipse (a = horizontal radius, b = vertical radius)
ellib(x, y, a, b, color)            -- ellipse border
rect(x, y, w, h, color)             -- filled rectangle
rectb(x, y, w, h, color)            -- rectangle border
line(x0, y0, x1, y1, color)         -- line
pset(x, y, color)                   -- pixel
```

### Sprites

```lua
spr(n, x, y)                        -- draw 8×8 sprite n at (x, y)
spr(n, x, y, cols, rows)            -- draw a block of cols×rows sprites starting at n
spr(n, x, y, cols, rows, flip_x, flip_y)
```

`cols` and `rows` — how many 8×8 sprites wide and tall to draw. Default 1×1.

Example: `spr(0, 10, 10, 2, 3)` draws a 16×24 block (2 sprites wide, 3 tall) starting from sprite 0.

### Palette

```lua
pal()                -- reset to default
pal(m0, m1, m2, m3)  -- remap indices: new[i] = old[m[i]]
```

### Input — keyboard

```lua
btn(id)    -- is button held
btnp(id)   -- was button just pressed this frame
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
local m = mouse()   -- current state (buttons held)
local m = mousep()  -- state at this frame (buttons just pressed)
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
function draw()
    cls(0)
    local m = mouse()
    circ(m.x, m.y, 2, 3)
    if m.left then rectb(m.x - 4, m.y - 4, 8, 8, 1) end
end
```

### Time

```lua
utime()    -- Unix timestamp in seconds (integer)
time()     -- milliseconds elapsed since game start (float)
```

- `utime()` — for idle games, persistent progress, saving timestamps.
- `time()` — for animations, timers, blinking.

```lua
-- animation
local t = time() / 1000
circ(80 + math.sin(t * 2) * 30, 80, 5, 3)

-- idle
local last = load(0)
local now = utime()
coins = coins + (now - last) * 0.1
save(0, now)
```

### Sound

```lua
sfx(n)             -- play sound n
sfx(n, volume)     -- play with volume 0.0–1.0
music(n)           -- play music n in loop
music(n, volume)   -- play with volume
music()            -- stop current music
```

### Save

```lua
save(pos, value)   -- write value (int64) to slot pos (0–63)
load(pos)          -- read value from slot pos
```

- On desktop: auto-saved to `save/storage.bin` on window close and every 10 minutes.
- On web: saved to browser `localStorage` immediately on every `save()`.

---

## Example

`data/scripts/main.lua`:

```lua
local x, y

function init()
    x = 80
    y = 80
end

function update()
    if btn(0) then x = x - 1 end
    if btn(1) then x = x + 1 end
    if btn(2) then y = y - 1 end
    if btn(3) then y = y + 1 end

    if btnp(4) then sfx(0) end
end

function draw()
    cls(0)
    spr(0, x - 4, y - 4)
    print("HELLO", 4, 4, 3)
end
```

---

## Credits

- [raylib](https://raylib.com) — zlib license
- [Lua 5.4.9](https://lua.org) — MIT license
- GB Chocolate palette by [GrafxKid](https://lospec.com/palette-list/gb-chocolate)

---

## License

MIT — see `LICENSE`.