#include "mxos.h"
static unsigned char bytes[6]={0xaa,0x12,0x34,0x56,0x78,0xbb};
int main(void)
{
 MXOS_FAR_ADDRESS p;
 mxos_u32 value=0;
 p.bank=0;p.offset=(mxos_u16)(bytes+1);
 if(mxos_get32_far(&p,&value)&MXOS_CF)return 1;
 if(value!=0x78563412UL)return 2;
 if(mxos_put32_far(&p,0x89abcdefUL)&MXOS_CF)return 3;
 if(bytes[0]!=0xaa||bytes[1]!=0xef||bytes[2]!=0xcd||bytes[3]!=0xab||bytes[4]!=0x89||bytes[5]!=0xbb)return 4;
 p.offset=0xfefd;value=0x12345678UL;
 if(!(mxos_get32_far(&p,&value)&MXOS_CF)||value!=0x12345678UL)return 5;
 if(!(mxos_put32_far(&p,0x89abcdefUL)&MXOS_CF))return 6;
 return 0;
}
