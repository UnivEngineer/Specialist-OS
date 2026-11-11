#include "mxos.h"
static unsigned char buffer[32];
int main(void)
{
    MXOS_FAR_ADDRESS p,q;
    MXOS_FAR_COPY c;
    mxos_u16 value,af;
    unsigned char byte;
    unsigned int i;
    if (sizeof(MXOS_FAR_ADDRESS)!=3) return 1;
    if (mxos_main_arena((void *)0x4000,0x1000)&MXOS_CF) return 2;
    if (mxos_malloc_far(MXOS_ALLOC_MAIN,32,&p)&MXOS_CF) return 3;
    if (p.bank || p.offset!=0x4000) return 4;
    if (mxos_put16_far(&p,0x1234)&MXOS_CF) return 5;
    if (mxos_get16_far(&p,&value)&MXOS_CF || value!=0x1234) return 6;
    if (mxos_put8_far(&p,0xCD)&MXOS_CF) return 7;
    if (mxos_get8_far(&p,&byte)&MXOS_CF || byte!=0xCD) return 8;
    if (mxos_realloc_far(&p,256)&MXOS_CF) return 9;
    if (mxos_get16_far(&p,&value)&MXOS_CF || value!=0x12CD) return 10;
    q=p;
    if (mxos_free_far(&p)&MXOS_CF || p.bank || p.offset) return 11;
    af=mxos_free_far(&q);
    if (!(af&MXOS_CF) || (af>>8)!=MXOS_MEM_ERR_OWNER) return 12;
    for (i=0;i<32;i++) buffer[i]=i;
    c.source_bank=c.destination_bank=0;
    c.source_offset=(mxos_u16)buffer; c.destination_offset=c.source_offset+1; c.bytes=31;
    if (mxos_memmove_far(&c)&MXOS_CF) return 13;
    for (i=1;i<32;i++) if (buffer[i]!=i-1) return 14;
    return 0;
}
