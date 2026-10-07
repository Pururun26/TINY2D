#pragma once

// Открыть лог рядом с exe. Вызывать один раз в начале main.
void tiny2d_log_init(void);

// Закрыть лог. Вызывать перед выходом.
void tiny2d_log_close(void);

// Свои логи (аналог printf, но в файл).
void tiny2d_log(const char *format, ...);