/* Отсортированный список свободных блоков с объединением соседей.
 * Заголовок: полный размер word, следующий offset word; FFFE — выданный блок.
 * Все операции локальны; DOS не вызывается. IRQ/потоки не обслуживаются. */
#include "mxos_heap.h"
#define NONE 65535u
#define LIVE 65534u
#define HEAD 4u
MXOS_HEAP mxos_default_heap;

static mxos_u16 word(MXOS_HEAP *h, mxos_u16 at)
{
    return (mxos_u16)h->base[at] | ((mxos_u16)h->base[at+1] << 8);
}
static void put(MXOS_HEAP *h, mxos_u16 at, mxos_u16 value)
{
    h->base[at]=(unsigned char)value;
    h->base[at+1]=(unsigned char)(value>>8);
}
static void link(MXOS_HEAP *h, mxos_u16 previous, mxos_u16 next)
{
    if (previous==NONE) h->first=next;
    else put(h,previous+2,next);
}
int mxos_heap_init(MXOS_HEAP *h, void *begin, mxos_u16 bytes)
{
    mxos_u16 address=(mxos_u16)begin;
    h->base=0; h->bytes=0; h->first=NONE;
    if (!address || bytes<6 || bytes>65535u-address) return 0;
    if (address&1) { ++address; --bytes; }
    bytes&=65534u;
    if (bytes<6) return 0;
    h->base=(unsigned char *)address; h->bytes=bytes; h->first=0;
    put(h,0,bytes); put(h,2,NONE);
    return 1;
}
void *mxos_heap_alloc(MXOS_HEAP *h, mxos_u16 bytes)
{
    mxos_u16 at,previous=NONE,size,next,need;
    if (!h->base || !bytes || bytes>65530u) return 0;
    need=((bytes+1)&65534u)+HEAD;
    at=h->first;
    while (at!=NONE) {
        size=word(h,at); next=word(h,at+2);
        if (size>=need) {
            if (size-need>=6) {
                put(h,at+need,size-need); put(h,at+need+2,next);
                link(h,previous,at+need); put(h,at,need);
            } else link(h,previous,next);
            put(h,at+2,LIVE);
            return h->base+at+HEAD;
        }
        previous=at; at=next;
    }
    return 0;
}
void mxos_heap_free(MXOS_HEAP *h, void *block)
{
    mxos_u16 at,previous=NONE,next,size;
    if (!h->base || !block) return;
    if ((mxos_u16)block<(mxos_u16)h->base+HEAD) return;
    at=(mxos_u16)block-(mxos_u16)h->base-HEAD;
    if ((at&1) || at>h->bytes-HEAD || word(h,at+2)!=LIVE) return;
    size=word(h,at);
    if (size<6 || size>h->bytes-at) return;
    next=h->first;
    while (next!=NONE && next<at) { previous=next; next=word(h,next+2); }
    if (next!=NONE && at+size==next) {
        size+=word(h,next); next=word(h,next+2);
    }
    put(h,at,size); put(h,at+2,next); link(h,previous,at);
    if (previous!=NONE && previous+word(h,previous)==at) {
        put(h,previous,word(h,previous)+size); put(h,previous+2,next);
    }
}
void *mxos_heap_realloc(MXOS_HEAP *h, void *block, mxos_u16 bytes)
{
    mxos_u16 at,old,i;
    unsigned char *result;
    if (!block) return mxos_heap_alloc(h,bytes);
    if (!bytes) { mxos_heap_free(h,block); return 0; }
    if (!h->base || (mxos_u16)block<(mxos_u16)h->base+HEAD) return 0;
    at=(mxos_u16)block-(mxos_u16)h->base-HEAD;
    if ((at&1) || at>h->bytes-HEAD || word(h,at+2)!=LIVE) return 0;
    old=word(h,at)-HEAD;
    if (bytes<=old) return block;
    result=mxos_heap_alloc(h,bytes);
    if (!result) return 0;
    for (i=0;i<old;++i) result[i]=((unsigned char *)block)[i];
    mxos_heap_free(h,block);
    return result;
}
void *mxos_heap_calloc(MXOS_HEAP *h, mxos_u16 count, mxos_u16 size)
{
    mxos_u16 total,i;
    unsigned char *block;
    if (!count || !size || count>65535u/size) return 0;
    total=count*size; block=mxos_heap_alloc(h,total);
    if (block) for (i=0;i<total;++i) block[i]=0;
    return block;
}
mxos_u16 mxos_heap_available(MXOS_HEAP *h, mxos_u16 *largest)
{
    mxos_u16 at,total=0,size;
    *largest=0;
    if (!h->base) return 0;
    at=h->first;
    while (at!=NONE) {
        size=word(h,at)-HEAD; total+=size;
        if (size>*largest) *largest=size;
        at=word(h,at+2);
    }
    return total;
}
