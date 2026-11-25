/* Проверка размеров FAR-пакета и C-переходника MEM на 8080. */
#include "mxos.h"
static MXOS_FAR_COPY request = {0,0x3000,0,0x4000,256};
int main(void) {
    mxos_u16 af, i;
    mxos_u8 *source=(mxos_u8*)0x3000, *target=(mxos_u8*)0x4000;
    for(i=0;i<256;i++) source[i]=(mxos_u8)(i*17+11);
    if(sizeof(MXOS_FAR_COPY)!=8) return 5;
    af=mxos_memcpy_far(&request);
    if((af & MXOS_CF) || (af >> 8)) return 1;
    for(i=0;i<256;i++) if(source[i]!=target[i]) return 2;
    request.destination_bank=17;
    af=mxos_memcpy_far(&request);
    if(!(af & MXOS_CF) || (af >> 8)!=0xe0) return 3;
    request.bytes=0;
    af=mxos_memcpy_far(&request);
    if((af & MXOS_CF) || (af >> 8)) return 4;

    request.source_offset=0x4000;
    request.destination_bank=0;
    request.destination_offset=0x4001;
    request.bytes=256;
    af=mxos_memcpy_far(&request);
    if((af & MXOS_CF) || (af >> 8)) return 6;
    for(i=0;i<256;i++) if(source[i]!=target[i+1]) return 7;

    request.source_offset=0x4001;
    request.destination_offset=0x4000;
    af=mxos_memmove_far(&request);
    if((af & MXOS_CF) || (af >> 8)) return 8;
    for(i=0;i<256;i++) if(source[i]!=target[i]) return 9;

    request.source_offset=0xFDFF;
    request.bytes=2;
    af=mxos_memcpy_far(&request);
    return (af & MXOS_CF) && (af >> 8)==MXOS_MEM_ERR_RANGE ? 0 : 10;
}
