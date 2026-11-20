#include "mxos_queue.h"
static MXOS_COMMAND_QUEUE q;
static mxos_u8 buffer[32];
static mxos_u16 handle,answer;
static void controller(void)
{
    MXOS_MODULE_REGS input;
    MXOS_REGS result;
    input.hl=0x1234; input.de=input.bc=input.af=0;
    mxos_route_call(handle,&input,&result);
    if (!(result.af&MXOS_CF)) answer=result.hl;
    if (mxos_command_select(&q,MXOS_QUEUE_DLL,handle)) return;
    q.next[0]=77; q.next[1]=q.next[2]=0;
    mxos_command_commit(&q,q.next+3);
}
int main(void)
{
    mxos_u8 *p,i;
    MXOS_MODULE_REGS input;
    MXOS_REGS result;
    if (mxos_command_open(&q,buffer,32,8)) return 1;
    for (i=0;i<7;++i) {
        p=q.next; *p++=MXOS_BIOS_SET_CURSOR_POS&255; *p++=5; *p++=MXOS_COMMAND_REG_HL; *p++=10; *p++=i;
        mxos_command_commit(&q,p);
    }
    if (mxos_upper('a')!='A' || q.used) return 2;
    if (mxos_command_reserve(&q,257) || q.error!=MXOS_MEM_ERR_RANGE) return 3;
    input.hl=0x1234; input.de=input.bc=input.af=0;
    mxos_route_call(0,&input,&result);
    if (!(result.af&MXOS_CF) || (result.af>>8)!=MXOS_MEM_ERR_HANDLE || input.hl!=0x1234) return 4;
    q.error=0;
    if (mxos_module_load("B:/QROUTE.DLL",MXOS_MODULE_MAIN,&handle)&MXOS_CF) return 5;
    if (mxos_module_scope(handle,controller) || answer!=0x1235 || q.used || q.error) return 6;
    if (mxos_module_unload(handle)&MXOS_CF) return 7;
    mxos_command_close(&q);
    return 0;
}
