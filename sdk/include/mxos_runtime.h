#ifndef MXOS_RUNTIME_H
#define MXOS_RUNTIME_H
#include "mxos_heap.h"
/* Заполняются crt0. Аргументы принадлежат банку приложения. */
extern char *mxos_args;
extern mxos_u8 mxos_home_page;
/* Два буфера одной ёмкости для банкового printf. (0, 0, 0) возвращает штатный режим. */
void mxos_printf_buffer(void *local, mxos_u16 main_address, mxos_u16 capacity);
#endif
