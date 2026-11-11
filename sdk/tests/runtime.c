#include <stdlib.h>
#include "mxos.h"
#include "mxos_runtime.h"
static unsigned int must_be_zero;
unsigned int runtime_result;
int main(void)
{
    unsigned char *p;
    MXOS_FAR_ADDRESS far;
    unsigned int i,af;
    if (must_be_zero) return 1;
    must_be_zero=1;
    p=calloc(128,2); if (!p) return 2;
    for (i=0;i<256;++i) if (p[i]) return 3;
    p[18]=42;
    if (p[18]!=42) return 4;
    af=mxos_calloc_far(MXOS_ALLOC_ANY_BANK,64,2,&far);
    if (af&MXOS_CF) return 5;
    if (mxos_put8_far(&far,42)&MXOS_CF) return 6;
    if (mxos_get8_far(&far,p)&MXOS_CF || p[0]!=42) return 7;
    if (mxos_free_far(&far)&MXOS_CF) return 8;
    free(p);
    mxos_puts("C heap: ready\r\n");
    runtime_result=0x55aa;
    return 0;
}
