#pragma once
#include <stdbool.h>

// Удаляет папку рекурсивно. Тихо, если папки нет.
void tiny2d_cart_remove(const char *dir);

// Проверяет, оканчивается ли путь на ".tiny"
bool tiny2d_cart_is_cart(const char *path);

// Распаковывает .tiny (ZIP) в out_dir.
// Внутри архива должна быть папка data/ — она распакуется как <out_dir>/data/...
// Возвращает true при успехе.
bool tiny2d_cart_extract(const char *cart_path, const char *out_dir);