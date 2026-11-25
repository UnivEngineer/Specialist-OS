/* Модульный API: возвращается AF диспетчера, AF модуля остаётся в regs. */
#include "mxos.h"
mxos_u16 mxos_module_load(const char *path,mxos_u8 target,mxos_u16 *handle)
{
    MXOS_CALL_PACKET r; mxos_u16 af;
    r.operation=MXOS_DOS_MODULE_LOAD; r.hl=(mxos_u16)path;
    r.de=r.bc=0; r.af=(mxos_u16)target<<8;
    af=mxos_call_packet(&r);
    if (!(af&MXOS_CF)) *handle=r.hl;
    return af;
}
mxos_u16 mxos_module_call(mxos_u16 handle,MXOS_MODULE_REGS *regs)
{
    MXOS_CALL_PACKET r;
    r.operation=MXOS_DOS_MODULE_CALL; r.hl=handle; r.de=(mxos_u16)regs;
    r.bc=r.af=0; return mxos_call_packet(&r);
}
mxos_u16 mxos_module_unload(mxos_u16 handle)
{
    MXOS_CALL_PACKET r;
    r.operation=MXOS_DOS_MODULE_UNLOAD; r.hl=handle; r.de=r.bc=r.af=0;
    return mxos_call_packet(&r);
}

mxos_u16 mxos_resident_retain(mxos_u16 key)
{
    MXOS_CALL_PACKET r; r.operation=MXOS_DOS_RESIDENT; r.af=0; r.de=key; r.hl=r.bc=0;
    return mxos_call_packet(&r);
}
mxos_u16 mxos_resident_find(mxos_u16 key,mxos_u16 *handle)
{
    MXOS_CALL_PACKET r; mxos_u16 af;
    r.operation=MXOS_DOS_RESIDENT; r.af=0x0100; r.de=key; r.hl=r.bc=0;
    af=mxos_call_packet(&r); if (!(af&MXOS_CF)) *handle=r.hl; return af;
}
mxos_u16 mxos_resident_next(mxos_u8 *index,mxos_u16 *handle,mxos_u16 *key,mxos_u8 *bank)
{
    MXOS_CALL_PACKET r; mxos_u16 af;
    r.operation=MXOS_DOS_RESIDENT; r.af=0x0200; r.bc=(mxos_u16)*index<<8; r.de=r.hl=0;
    af=mxos_call_packet(&r);
    if (!(af&MXOS_CF)) { *index=r.bc>>8; *bank=r.bc; *handle=r.hl; *key=r.de; }
    return af;
}

static mxos_u16 resident_event(mxos_u16 handle,mxos_u8 event)
{
    MXOS_MODULE_REGS r; mxos_u16 af;
    r.hl=r.de=r.bc=0; r.af=(mxos_u16)event<<8;
    af=mxos_module_call(handle,&r);
    return (af&MXOS_CF)?af:r.af;
}
mxos_u16 mxos_resident_restore(mxos_u16 handle) { return resident_event(handle,1); }
mxos_u16 mxos_resident_close(mxos_u16 handle)
{
    mxos_u16 af=resident_event(handle,2);
    if (af&MXOS_CF) return af;
    return mxos_module_unload(handle);
}
mxos_u16 mxos_resident_kill(mxos_u16 handle) { return mxos_module_unload(handle); }

mxos_u16 mxos_shell_resident(mxos_u16 key)
{
    MXOS_CALL_PACKET r; r.operation=MXOS_DOS_RESIDENT;
    r.af=MXOS_RESIDENT_SHELL_SET<<8; r.de=key; r.hl=r.bc=0;
    return mxos_call_packet(&r);
}
mxos_u16 mxos_shell_resume(void)
{
    MXOS_CALL_PACKET r; r.operation=MXOS_DOS_RESIDENT;
    r.af=MXOS_RESIDENT_SHELL_RESUME<<8; r.de=r.hl=r.bc=0;
    return mxos_call_packet(&r);
}
mxos_u16 mxos_directory_epoch(mxos_u16 *low,mxos_u16 *high)
{
    MXOS_CALL_PACKET r; mxos_u16 af; r.operation=MXOS_DOS_RESIDENT;
    r.af=MXOS_RESIDENT_DIRECTORY_EPOCH<<8; r.de=r.hl=r.bc=0;
    af=mxos_call_packet(&r);
    if (!(af&MXOS_CF)) { *low=r.hl; *high=r.de; }
    return af;
}
mxos_u16 mxos_drive_epochs(mxos_u8 *epochs)
{
    MXOS_CALL_PACKET r; r.operation=MXOS_DOS_RESIDENT;
    r.af=MXOS_RESIDENT_DIRECTORY_EPOCH<<8; r.hl=(mxos_u16)epochs;
    r.bc=MXOS_RESIDENT_EPOCH_ALL<<8; r.de=0;
    return mxos_call_packet(&r);
}
mxos_u16 mxos_resident_release(void)
{
    MXOS_CALL_PACKET r; r.operation=MXOS_DOS_RESIDENT;
    r.af=MXOS_RESIDENT_RELEASE<<8; r.de=r.hl=r.bc=0;
    return mxos_call_packet(&r);
}

mxos_u16 mxos_fileops(mxos_u8 operation,MXOS_FILEOPS_JOB *job)
{
    MXOS_CALL_PACKET r; r.operation=MXOS_DOS_FILEOPS; r.hl=(mxos_u16)job;
    r.de=r.bc=0; r.af=(mxos_u16)operation<<8;
    return mxos_call_packet(&r);
}

mxos_u16 mxos_enumerate(mxos_u8 operation,MXOS_ENUM_REQUEST *request)
{
    MXOS_CALL_PACKET r; r.operation=MXOS_DOS_ENUMERATE; r.hl=(mxos_u16)request;
    r.de=r.bc=0; r.af=(mxos_u16)operation<<8;
    return mxos_call_packet(&r);
}
