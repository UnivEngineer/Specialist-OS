/* Необязательный backend stdlib. До первого malloc runtime задаёт диапазон
 * mxos_default_heap через mxos_heap_init; он не должен принадлежать ARENA. */
#include <stdlib.h>
#include "mxos_heap.h"
void *malloc(size_t bytes) { return mxos_heap_alloc(&mxos_default_heap,bytes); }
void free(void *block) { mxos_heap_free(&mxos_default_heap,block); }
void *realloc(void *block,size_t bytes) { return mxos_heap_realloc(&mxos_default_heap,block,bytes); }
void *calloc(size_t count,size_t size) { return mxos_heap_calloc(&mxos_default_heap,count,size); }
