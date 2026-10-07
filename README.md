# TINY2D

A tiny retro game engine for Lua. Built for making small 2D games fast.

- 160×160 virtual screen, 4× window
- 4-color palette (GB Chocolate) with runtime palette swapping
- Sprites from indexed PNG, transparency via magenta `#FF00FF`
- Sound and music loaded from files
- Save system — 64 slots × 8 bytes
- Lua scripting with `require`
- Single ~1 MB executable, no external DLLs

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

---

## Running

`tiny2d.exe` expects a `data/` folder **next to it**. If `data/` is missing, the engine has nothing to run.

To start, download the release `.zip` (which includes `data/`) or create your own `data/` folder — see the structure below.

---

## Architecture

```
tiny2d.exe
data/
├── scripts/
│   └── main.lua          ← game entry point
├── sprites/
│   ├── font_pico8.png    ← 128×128 font sheet (3×5 glyphs)
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

- **`font_pico8.png`** — 128×128 font sheet, glyphs 3×5 px.
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
function update()   -- logic, called 60 times per second
end

function draw()     -- rendering, called 60 times per second
end
```

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
elli(x, y, a, b, color)             -- filled ellipse
ellib(x, y, a, b, color)            -- ellipse border
rect(x, y, w, h, color)             -- filled rectangle
rectb(x, y, w, h, color)            -- rectangle border
line(x0, y0, x1, y1, color)         -- line
pset(x, y, color)                   -- pixel
```

### Sprites

```lua
spr(n, x, y)                        -- draw sprite n at (x, y)
spr(n, x, y, w, h)                  -- draw w×h block of sprites
spr(n, x, y, w, h, flip_x, flip_y)  -- with flipping
```

### Palette

```lua
pal()                -- reset to default
pal(m0, m1, m2, m3)  -- remap indices: new[i] = old[m[i]]
```

### Input

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

Auto-saved to `save/storage.bin` on window close and every 10 minutes.

---

## Example

`data/scripts/main.lua`:

```lua
local x, y = 80, 80

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
