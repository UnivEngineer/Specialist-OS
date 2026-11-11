/* FAR-память: блоки распределяет DOS, доступ к байтам выполняет локальный MEM. */
#include "mxos.h"

static mxos_u16 memory_call(MXOS_CALL *r, mxos_u8 op, mxos_u8 bank, mxos_u16 address, mxos_u16 value)
{
    r->operation=MXOS_DOS_MEMORY; r->af=(mxos_u16)op<<8;
    r->bc=(mxos_u16)bank<<8; r->hl=address; r->de=value;
    return mxos_call(r);
}
mxos_u16 mxos_malloc_far(mxos_u8 bank, mxos_u16 bytes, MXOS_FAR_ADDRESS *out)
{
    MXOS_CALL r; mxos_u16 af;
    af=memory_call(&r,MXOS_MEM_OP_ALLOC,bank,bytes,0);
    if (!(af & MXOS_CF)) { out->bank=r.bc>>8; out->offset=r.hl; }
    return af;
}
mxos_u16 mxos_free_far(MXOS_FAR_ADDRESS *address)
{
    MXOS_CALL r; mxos_u16 af;
    af=memory_call(&r,MXOS_MEM_OP_FREE,address->bank,address->offset,0);
    if (!(af & MXOS_CF)) { address->bank=0; address->offset=0; }
    return af;
}
mxos_u16 mxos_realloc_far(MXOS_FAR_ADDRESS *address, mxos_u16 bytes)
{
    MXOS_CALL r; mxos_u16 af;
    af=memory_call(&r,MXOS_MEM_OP_REALLOC,address->bank,address->offset,bytes);
    if (!(af & MXOS_CF)) { address->bank=r.bc>>8; address->offset=r.hl; }
    return af;
}
mxos_u16 mxos_main_arena(void *start, mxos_u16 bytes)
{
    MXOS_CALL r; return memory_call(&r,MXOS_MEM_OP_ARENA,0,(mxos_u16)start,bytes);
}
static mxos_u16 access(MXOS_FAR_ADDRESS *p, mxos_u8 op, mxos_u16 value, mxos_u16 *out)
{
    MXOS_REGS r;
    r.entry=MXOS_MEM_SERVICE; r.af=(mxos_u16)op<<8;
    r.bc=(mxos_u16)p->bank<<8; r.hl=p->offset; r.de=value;
    mxos_bios_call(&r);
    if (!(r.af & MXOS_CF) && out) *out=r.de;
    return r.af;
}
mxos_u16 mxos_get8_far(MXOS_FAR_ADDRESS *p, mxos_u8 *out)
{
    mxos_u16 value,af; af=access(p,MXOS_MEM_OP_GET8,0,&value);
    if (!(af & MXOS_CF)) *out=value;
    return af;
}
mxos_u16 mxos_get16_far(MXOS_FAR_ADDRESS *p, mxos_u16 *out) { return access(p,MXOS_MEM_OP_GET16,0,out); }
mxos_u16 mxos_put8_far(MXOS_FAR_ADDRESS *p, mxos_u8 value) { return access(p,MXOS_MEM_OP_PUT8,value,0); }
mxos_u16 mxos_put16_far(MXOS_FAR_ADDRESS *p, mxos_u16 value) { return access(p,MXOS_MEM_OP_PUT16,value,0); }

mxos_u16 mxos_fill_far(MXOS_FAR_ADDRESS *p, mxos_u8 value, mxos_u16 bytes)
{
    MXOS_CALL r;
    r.operation=MXOS_DOS_MEMORY; r.af=MXOS_MEM_OP_FILL<<8;
    r.bc=((mxos_u16)p->bank<<8)|value; r.hl=p->offset; r.de=bytes;
    return mxos_call(&r);
}
mxos_u16 mxos_calloc_far(mxos_u8 bank,mxos_u16 count,mxos_u16 size,MXOS_FAR_ADDRESS *out)
{
    MXOS_FAR_ADDRESS p; mxos_u16 af,bytes;
    if (size && count>65535u/size) return (MXOS_MEM_ERR_NOMEM<<8)|MXOS_CF;
    bytes=count*size; af=mxos_malloc_far(bank,bytes,&p);
    if (af&MXOS_CF) return af;
    af=mxos_fill_far(&p,0,bytes);
    if (af&MXOS_CF) { mxos_free_far(&p); return af; }
    *out=p; return af;
}
mxos_u16 mxos_memory_query(mxos_u8 bank,MXOS_MEMORY_INFO *out)
{
    MXOS_CALL r; mxos_u16 af;
    af=memory_call(&r,MXOS_MEM_OP_QUERY,bank,0,0);
    if (!(af&MXOS_CF)) { out->free_bytes=r.hl; out->largest=r.de; out->free_records=r.bc; }
    return af;
}
mxos_u16 mxos_ramdisk_status(mxos_u8 *pages,mxos_u8 *state)
{
    MXOS_CALL r; mxos_u16 af;
    r.operation=MXOS_DOS_MEMORY; r.af=MXOS_MEM_OP_RAMDISK<<8;
    r.bc=MXOS_RAMDISK_STATUS; r.hl=r.de=0;
    af=mxos_call(&r);
    if (!(af&MXOS_CF)) { *pages=r.bc>>8; *state=r.bc; }
    return af;
}
mxos_u16 mxos_ramdisk_commit(void)
{
    MXOS_CALL r;
    r.operation=MXOS_DOS_MEMORY; r.af=MXOS_MEM_OP_RAMDISK<<8;
    r.bc=MXOS_RAMDISK_COMMIT; r.hl=r.de=0;
    return mxos_call(&r);
}

/* DWORD проходит через регистры нового локального MEM ABI, без DOS. */
mxos_u16 mxos_get32_far(MXOS_FAR_ADDRESS *p, mxos_u32 *out)
{
    MXOS_REGS r;
    r.entry=MXOS_MEM_GET32; r.af=(mxos_u16)p->bank<<8;
    r.hl=p->offset; r.bc=r.de=0;
    mxos_bios_call(&r);
    if (!(r.af & MXOS_CF)) *out=((mxos_u32)r.bc<<16)|r.de;
    return r.af;
}
mxos_u16 mxos_put32_far(MXOS_FAR_ADDRESS *p, mxos_u32 value)
{
    MXOS_REGS r;
    r.entry=MXOS_MEM_PUT32; r.af=(mxos_u16)p->bank<<8;
    r.hl=p->offset; r.bc=(mxos_u16)(value>>16); r.de=(mxos_u16)value;
    mxos_bios_call(&r);
    return r.af;
}
