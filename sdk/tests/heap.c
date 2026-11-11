/* Один и тот же NEAR-аллокатор запускается из main и EXEC-банка. */
#include <stdlib.h>
#include "mxos_heap.h"
static unsigned char arena[2048];
int main(void)
{
    unsigned char *a,*b,*c;
    unsigned int i,largest;
    if (!mxos_heap_init(&mxos_default_heap,arena,sizeof(arena))) return 1;
    a=malloc(63); b=calloc(32,2);
    if (!a || !b || ((unsigned int)a&1)) return 2;
    for (i=0;i<64;++i) if (b[i]) return 3;
    for (i=0;i<63;++i) a[i]=(unsigned char)i;
    c=realloc(a,130);
    if (!c) return 4;
    for (i=0;i<63;++i) if (c[i]!=i) return 5;
    if (calloc(32769u,2) || malloc(65535u)) return 6;
    if (realloc(c,4096)) return 7;
    for (i=0;i<63;++i) if (c[i]!=i) return 8;
    free(c); free(b); free(0);
    if (mxos_heap_available(&mxos_default_heap,&largest)!=2044 || largest!=2044) return 9;
    a=malloc(2044); if (!a || malloc(1)) return 10;
    free(a);
    if (malloc(0) || calloc(0,4)) return 11;
    a=malloc(16); if (realloc(a,0)) return 12;
    return 0;
}
