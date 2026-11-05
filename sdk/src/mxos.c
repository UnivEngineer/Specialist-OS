/* Регистровые обёртки уже реализованных служб; стандартные потоки задаёт CLI. */
#include "mxos.h"

static void prepare(MXOS_REGS *r, mxos_u16 entry)
{
    r->entry=entry; r->af=r->bc=r->de=r->hl=0;
}
static mxos_u8 result(MXOS_REGS *r)
{
    mxos_u8 error;
    mxos_bios_call(r);
    if (!(r->af & MXOS_CF)) return 0;
    error=mxos_last_error(); return error ? error : 1;
}
void *mxos_memcpy(void *dst, const void *src, mxos_u16 bytes)
{
    MXOS_REGS r;
    prepare(&r,MXOS_BIOS_MEMCPY);
    r.hl=(mxos_u16)src; r.de=r.hl+bytes; r.bc=(mxos_u16)dst;
    mxos_bios_call(&r); return dst;
}
void *mxos_memmove(void *dst, const void *src, mxos_u16 bytes) { return mxos_memcpy(dst,src,bytes); }
mxos_u8 mxos_directory_get(mxos_u16 *cluster)
{
    MXOS_REGS r; mxos_u8 error;
    prepare(&r,MXOS_BIOS_DIRECTORY); error=result(&r);
    if (!error) *cluster=r.de;
    return error;
}
mxos_u8 mxos_directory_set(mxos_u16 cluster)
{
    MXOS_REGS r; prepare(&r,MXOS_BIOS_DIRECTORY); r.af=0x0100; r.de=cluster;
    return result(&r);
}
mxos_u8 mxos_getcwd(char path[MXOS_MAX_PATH])
{
    MXOS_REGS r; prepare(&r,MXOS_BIOS_DIRECTORY); r.af=0x0400; r.hl=(mxos_u16)path;
    return result(&r);
}
mxos_u8 mxos_chdir(const char path[MXOS_MAX_PATH])
{
    MXOS_REGS r; prepare(&r,MXOS_BIOS_DIRECTORY); r.af=0x0200; r.hl=(mxos_u16)path;
    return result(&r);
}
mxos_u8 mxos_mkdir(const char path[MXOS_MAX_PATH])
{
    MXOS_REGS r; prepare(&r,MXOS_BIOS_DIRECTORY); r.af=0x0700; r.hl=(mxos_u16)path;
    return result(&r);
}
mxos_u8 mxos_rmdir(const char path[MXOS_MAX_PATH])
{
    MXOS_REGS r; prepare(&r,MXOS_BIOS_DIRECTORY); r.af=0x0800; r.hl=(mxos_u16)path;
    return result(&r);
}
mxos_u8 mxos_file_move(void *request)
{
    MXOS_REGS r; prepare(&r,MXOS_BIOS_CLI); r.af=0x0700; r.hl=(mxos_u16)request;
    return result(&r);
}
mxos_u8 mxos_file_getattr(const void *name83, mxos_u8 *attributes)
{
    MXOS_REGS r; mxos_u8 error;
    prepare(&r,MXOS_BIOS_FILE_ATTR); r.hl=(mxos_u16)name83; error=result(&r);
    if (!error) *attributes=r.af>>8;
    return error;
}
mxos_u8 mxos_file_setattr(const void *name83, mxos_u8 attributes)
{
    MXOS_REGS r; prepare(&r,MXOS_BIOS_FILE_ATTR); r.hl=(mxos_u16)name83;
    r.bc=((mxos_u16)attributes<<8)|1; return result(&r);
}
mxos_u8 mxos_directory_list(MXOS_FILE_INFO *buffer, mxos_u16 capacity, mxos_u16 start, mxos_u16 *total)
{
    MXOS_REGS r; mxos_u8 error;
    prepare(&r,MXOS_BIOS_DIRECTORY); r.af=0x0300; r.hl=(mxos_u16)buffer; r.de=capacity; r.bc=start;
    error=result(&r); *total=r.de; return error;
}
mxos_u16 mxos_checksum(const void *buffer, mxos_u16 bytes)
{
    MXOS_REGS r;
    if (!bytes) return 0;
    prepare(&r,MXOS_BIOS_CHECKSUM); r.hl=(mxos_u16)buffer; r.de=r.hl+bytes-1;
    mxos_bios_call(&r); return r.bc;
}
#ifndef __Z88DK
/* Для sccz80 горячие однорегистровые вызовы находятся в коротком ASM-мосте. */
mxos_u16 mxos_memtop(void) { MXOS_REGS r; prepare(&r,MXOS_BIOS_MEMTOP_GET); mxos_bios_call(&r); return r.hl; }
void mxos_set_memtop(mxos_u16 top) { MXOS_REGS r; prepare(&r,MXOS_BIOS_MEMTOP_SET); r.hl=top; mxos_bios_call(&r); }
void mxos_release_module(void) { MXOS_REGS r; prepare(&r,MXOS_BIOS_MODULE_RELEASE); mxos_bios_call(&r); }
mxos_u16 mxos_getkey(void) { MXOS_REGS r; prepare(&r,MXOS_BIOS_GETCH); mxos_bios_call(&r); return r.af; }
int mxos_getch(void) { return mxos_getkey()>>8; }
int mxos_keyscan(void)
{
    MXOS_REGS r; mxos_u8 ch;
    prepare(&r,MXOS_BIOS_KEYSCAN); mxos_bios_call(&r); ch=(mxos_u8)(r.af>>8);
    return ch==255 ? -1 : ch;
}
void mxos_putch(mxos_u8 ch) { MXOS_REGS r; prepare(&r,MXOS_BIOS_PUTCH); r.bc=ch; mxos_bios_call(&r); }
void mxos_puts(const char *text) { MXOS_REGS r; prepare(&r,MXOS_BIOS_PUTS); r.hl=(mxos_u16)text; mxos_bios_call(&r); }
void mxos_cls(void) { mxos_putch(31); }
mxos_u16 mxos_cursor_get(void) { MXOS_REGS r; prepare(&r,MXOS_BIOS_CURSOR_GET); mxos_bios_call(&r); return r.hl; }
void mxos_cursor_set(mxos_u16 xy) { MXOS_REGS r; prepare(&r,MXOS_BIOS_CURSOR_SET); r.hl=xy; mxos_bios_call(&r); }
mxos_u8 mxos_upper(mxos_u8 ch) { MXOS_REGS r; prepare(&r,MXOS_BIOS_UPPER); r.af=(mxos_u16)ch<<8; mxos_bios_call(&r); return (mxos_u8)(r.af>>8); }
#endif
void mxos_print_dec(mxos_u16 value, mxos_u8 padding) { MXOS_REGS r; prepare(&r,MXOS_BIOS_PRINT_DEC); r.de=value; r.bc=(mxos_u16)padding<<8; mxos_bios_call(&r); }
void mxos_print_hex8(mxos_u8 value) { MXOS_REGS r; prepare(&r,MXOS_BIOS_PRINT_HEX8); r.af=(mxos_u16)value<<8; mxos_bios_call(&r); }
void mxos_print_hex16(mxos_u16 value) { MXOS_REGS r; prepare(&r,MXOS_BIOS_PRINT_HEX16); r.hl=value; mxos_bios_call(&r); }
mxos_u16 mxos_readline(char *buffer, mxos_u16 capacity)
{
    MXOS_REGS r; mxos_u16 n=0;
    if (!capacity) return 0;
    prepare(&r,MXOS_BIOS_INPUT); r.hl=(mxos_u16)buffer; r.de=r.hl+capacity-1;
    mxos_bios_call(&r);
    while (n<capacity-1 && buffer[n] && buffer[n]!=13) ++n;
    buffer[n]=0; return n;
}
int mxos_equal_n(const char *a, const char *b, mxos_u8 bytes, int ignore_case)
{
    MXOS_REGS r;
    if (!bytes) return 1;
    prepare(&r,ignore_case ? MXOS_BIOS_STRICMP : MXOS_BIOS_STRCMP);
    r.de=(mxos_u16)a; r.hl=(mxos_u16)b; r.bc=bytes;
    mxos_bios_call(&r); return (r.af & MXOS_ZF)!=0;
}
#ifndef __Z88DK
void mxos_timer_init(void) { MXOS_REGS r; prepare(&r,MXOS_BIOS_TIMER_INIT); mxos_bios_call(&r); }
mxos_u16 mxos_timer_read(void) { MXOS_REGS r; prepare(&r,MXOS_BIOS_TIMER_READ); mxos_bios_call(&r); return r.hl; }
#endif
mxos_u8 mxos_drive_select(mxos_u8 drive) { MXOS_REGS r; prepare(&r,MXOS_BIOS_DRIVE_SELECT); r.af=(mxos_u16)drive<<8; return result(&r); }
mxos_u8 mxos_volume_flush(void) { MXOS_REGS r; prepare(&r,MXOS_BIOS_FLUSH); return result(&r); }
mxos_u8 mxos_last_error(void) { MXOS_REGS r; prepare(&r,MXOS_BIOS_ERROR); mxos_bios_call(&r); return (mxos_u8)(r.af>>8); }
const MXOS_DISK_INFO *mxos_disk_info(mxos_u16 *physical_blocks)
{
    MXOS_REGS r; prepare(&r,MXOS_BIOS_DISK_DRIVER); r.bc=0x0300; mxos_bios_call(&r);
    if (physical_blocks) *physical_blocks=r.de;
    return (r.af & MXOS_CF) ? 0 : (const MXOS_DISK_INFO *)r.hl;
}
mxos_u8 mxos_file_list(MXOS_FILE_INFO *buffer, mxos_u16 capacity, mxos_u16 start, mxos_u16 *total)
{
    MXOS_REGS r; mxos_u8 error;
    prepare(&r,MXOS_BIOS_FILE_LIST); r.hl=(mxos_u16)buffer; r.de=capacity; r.bc=start;
    error=result(&r); if (total) *total=r.de; return error;
}
static mxos_u8 file_call(mxos_u16 entry, const void *descriptor, const void *buffer)
{
    MXOS_REGS r; mxos_u8 error;
    prepare(&r,entry); r.hl=(mxos_u16)descriptor; r.de=(mxos_u16)buffer;
    mxos_bios_call(&r);
    if (!(r.af & MXOS_CF)) return 0;
    error=mxos_last_error(); return error ? error : 1;
}
mxos_u8 mxos_file_stat(void *d) { return file_call(MXOS_BIOS_FILE_STAT,d,0); }
mxos_u8 mxos_file_load(void *d, void *b) { return file_call(MXOS_BIOS_FILE_LOAD,d,b); }
mxos_u8 mxos_file_save(void *d, const void *b) { return file_call(MXOS_BIOS_FILE_CREATE,d,b); }
mxos_u8 mxos_file_delete(const void *n) { return file_call(MXOS_BIOS_FILE_DELETE,n,0); }
mxos_u8 mxos_file_rename(const void *a, const void *b) { return file_call(MXOS_BIOS_FILE_RENAME,a,b); }
mxos_u8 mxos_file_copy(void *q) { return file_call(MXOS_BIOS_FILE_COPY,q,0); }
mxos_u8 mxos_sector_io(mxos_u8 op, mxos_u16 sector, void *b)
{
    MXOS_REGS r; prepare(&r,MXOS_BIOS_SECTOR); r.bc=(mxos_u16)op<<8; r.de=sector; r.hl=(mxos_u16)b;
    return result(&r);
}
void mxos_ui_draw(const void *p, mxos_u8 x) { MXOS_REGS r; prepare(&r,MXOS_BIOS_UI_DRAW); r.hl=(mxos_u16)p; r.af=(mxos_u16)x<<8; mxos_bios_call(&r); }
void mxos_ui_inverse(void *p, mxos_u8 w, mxos_u8 h) { MXOS_REGS r; prepare(&r,MXOS_BIOS_UI_INVERSE); r.hl=(mxos_u16)p; r.bc=((mxos_u16)w<<8)|h; mxos_bios_call(&r); }
void mxos_ui_copy(const void *a, void *b, mxos_u8 w, mxos_u8 h) { MXOS_REGS r; prepare(&r,MXOS_BIOS_UI_COPY); r.hl=(mxos_u16)a; r.bc=(mxos_u16)b; r.de=((mxos_u16)w<<8)|h; mxos_bios_call(&r); }
