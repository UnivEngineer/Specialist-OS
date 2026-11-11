/* Локальная NEAR-куча. Контекст, заголовки и блоки находятся в одном банке. */
#ifndef MXOS_HEAP_H
#define MXOS_HEAP_H
#include "mxos_file.h"
typedef struct {
    unsigned char *base;
    mxos_u16 bytes, first;
} MXOS_HEAP;
/* Ноль — отказ. Диапазон передаётся владельцем и исключает код/стек/MEM. */
int mxos_heap_init(MXOS_HEAP *heap, void *begin, mxos_u16 bytes);
void *mxos_heap_alloc(MXOS_HEAP *heap, mxos_u16 bytes);
void mxos_heap_free(MXOS_HEAP *heap, void *block);
void *mxos_heap_realloc(MXOS_HEAP *heap, void *block, mxos_u16 bytes);
void *mxos_heap_calloc(MXOS_HEAP *heap, mxos_u16 count, mxos_u16 size);
mxos_u16 mxos_heap_available(MXOS_HEAP *heap, mxos_u16 *largest);
/* Используется дополнительным mxos_stdlib.c. Нулевой контекст до init безопасен. */
extern MXOS_HEAP mxos_default_heap;
#endif
