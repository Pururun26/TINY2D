local cam_x = 0
local cam_y = 0

-- Сохраняем оригиналы --
local _print = print
local _circ  = circ
local _circb = circb
local _elli  = elli
local _ellib = ellib
local _rect  = rect
local _rectb = rectb
local _line  = line
local _pset  = pset
local _spr   = spr

-- Установка камеры --
function camera(x, y)
    cam_x = x or 0
    cam_y = y or 0
end

-- Graphics --
function print(text, x, y, color)
    _print(text, x - cam_x, y - cam_y, color)
end

-- Примитивы --
function circ(x, y, r, color)
    _circ(x - cam_x, y - cam_y, r, color)
end

function circb(x, y, r, color)
    _circb(x - cam_x, y - cam_y, r, color)
end

function elli(x, y, a, b, color)
    _elli(x - cam_x, y - cam_y, a, b, color)
end

function ellib(x, y, a, b, color)
    _ellib(x - cam_x, y - cam_y, a, b, color)
end

function rect(x, y, w, h, color)
    _rect(x - cam_x, y - cam_y, w, h, color)
end

function rectb(x, y, w, h, color)
    _rectb(x - cam_x, y - cam_y, w, h, color)
end

function line(x0, y0, x1, y1, color)
    _line(x0 - cam_x, y0 - cam_y, x1 - cam_x, y1 - cam_y, color)
end

function pset(x, y, color)
    _pset(x - cam_x, y - cam_y, color)
end

-- Спрайт --
function spr(n, x, y, cols, rows, flip_x, flip_y)
    _spr(n, x - cam_x, y - cam_y,
         cols or 1, rows or 1,
         flip_x or false, flip_y or false)
end