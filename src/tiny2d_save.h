#pragma once
#include <stdint.h>
#include <stdbool.h>

// Инициализация
void tiny2d_save_init(void);

// Чтение/запись (в память)
void    tiny2d_save(int pos, int64_t value);
int64_t tiny2d_load(int pos);

// Запись всего на диск
bool tiny2d_save_all(void);