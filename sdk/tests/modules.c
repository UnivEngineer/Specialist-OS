#include "mxos.h"
static char bank_path[MXOS_MAX_PATH]="B:\\BANKMOD.DLL";
static char main_path[MXOS_MAX_PATH]="B:\\MAINMOD.DLL";
static char dos_path[MXOS_MAX_PATH]="B:\\DOSMOD.DLL";
int main(void)
{
    mxos_u16 handle,old,af;
    mxos_u8 pages,state;
    MXOS_MODULE_REGS r;
    if (sizeof(r)!=8) return 1;
    if (mxos_ramdisk_status(&pages,&state)&MXOS_CF) return 16;
    if (pages!=4 || state!=1) return 17;
    af=mxos_ramdisk_commit();
    if (!(af&MXOS_CF) || (af>>8)!=MXOS_MEM_ERR_OWNER) return 18;
    if (mxos_main_arena((void *)0x6000,0x2000)&MXOS_CF) return 2;
    if (mxos_module_load(bank_path,MXOS_MODULE_BANK,&handle)&MXOS_CF) return 3;
    r.hl=0x1234;r.de=0x2345;r.bc=0x3456;r.af=0;
    if (mxos_module_call(handle,&r)&MXOS_CF) return 4;
    if (r.hl!=0x1235 || r.de!=0x2346 || r.bc!=0x3457 || !(r.af&MXOS_CF)) return 5;
    old=handle;
    if (mxos_module_unload(handle)&MXOS_CF) return 6;
    af=mxos_module_call(old,&r);
    if (!(af&MXOS_CF) || (af>>8)!=MXOS_MEM_ERR_HANDLE) return 7;
    if (mxos_module_load(main_path,MXOS_MODULE_MAIN,&handle)&MXOS_CF) return 8;
    r.af=0;
    if (mxos_module_call(handle,&r)&MXOS_CF) return 9;
    if (r.hl!=0x1236) return 10;
    if (mxos_module_unload(handle)&MXOS_CF) return 11;
    if (mxos_module_load(dos_path,MXOS_MODULE_DOS,&handle)&MXOS_CF) return 12;
    r.af=0;
    if (mxos_module_call(handle,&r)&MXOS_CF) return 13;
    if (r.hl!=0x1237) return 14;
    if (mxos_module_unload(handle)&MXOS_CF) return 15;
    return 0;
}
