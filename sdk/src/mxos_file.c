/* Адаптер для stdlib: возвращает элементы fread/fwrite, ядро считает байты. */
#include "mxos_file.h"

static void put16(mxos_u8 *p, mxos_u16 n)
{
    p[0]=(mxos_u8)n; p[1]=(mxos_u8)(n>>8);
}
static mxos_u16 get16(const mxos_u8 *p)
{
    return (mxos_u16)p[0] | ((mxos_u16)p[1]<<8);
}
static int invoke(MXOS_FILE *f, mxos_u8 op)
{
    mxos_u8 error;
    f->bytes[63]=op;
    error=mxos_file_call(f);
    if (error) f->bytes[43]=error;
    return error ? -1 : 0;
}
static int argument(MXOS_FILE *f)
{
    f->bytes[43]=7;
    return -1;
}

int mxos_fopen(MXOS_FILE *f, const char *path, const char *mode)
{
    mxos_u8 flags, i, n, field, ch;
    /* Без префикса используется текущий диск DOS, а не скрытое A:. */
    for (i=0;i<64;++i) f->bytes[i]=0;
    for (i=0;i<11;++i) f->bytes[i]=' ';
    f->bytes[40]=mxos_current_drive();
    if (!path || !mode) return argument(f);
    switch (*mode++) {
    case 'r': flags=1; break;
    case 'w': flags=14; break;
    case 'a': flags=22; break;
    default: return argument(f);
    }
    n=0; field=0;
    while (*mode) {
        ch=(mxos_u8)*mode++;
        if (ch=='+') { if (n) return argument(f); n=1; flags|=3; }
        else if (ch=='b') { if (field) return argument(f); field=1; }
        else return argument(f);
    }
    if (path[0] && path[1]==':') {
        ch=(mxos_u8)path[0];
        if (ch>='a' && ch<='h') ch-=32;
        if (ch<'A' || ch>'H') return argument(f);
        f->bytes[40]=ch-'A'; path+=2;
    }
    n=0; field=0;
    while (*path) {
        ch=(mxos_u8)*path++;
        if (ch=='.') {
            if (!n || field) return argument(f);
            field=1; n=0;
        } else {
            if (ch<33 || ch=='/' || ch=='\\' || ch==':' || ch=='*' || ch=='?' || ch=='"' || ch=='<' || ch=='>' || ch=='|') return argument(f);
            if (n==(field ? 3 : 8)) return argument(f);
            f->bytes[(field ? 8 : 0)+n++]=ch;
        }
    }
    if (!n || f->bytes[0]==' ') return argument(f);
    f->bytes[41]=flags;
    return invoke(f,0);
}

static mxos_u16 transfer(MXOS_FILE *f, void *buffer, mxos_u16 bytes, mxos_u8 op)
{
    put16(f->bytes+49,(mxos_u16)buffer);
    put16(f->bytes+51,bytes);
    if (invoke(f,op)<0 && (f->bytes[43]==7 || f->bytes[43]==10)) return 0;
    return get16(f->bytes+51);
}
mxos_u16 mxos_read(MXOS_FILE *f, void *b, mxos_u16 n) { return transfer(f,b,n,1); }
mxos_u16 mxos_write(MXOS_FILE *f, const void *b, mxos_u16 n) { return transfer(f,(void *)b,n,2); }

static mxos_u16 elements(MXOS_FILE *f, void *b, mxos_u16 size, mxos_u16 count, mxos_u8 op)
{
    mxos_u32 total;
    if (!size || !count) return 0;
    total=(mxos_u32)size*count;
    if (total>65535UL) { argument(f); return 0; }
    return transfer(f,b,(mxos_u16)total,op)/size;
}
mxos_u16 mxos_fread(void *b, mxos_u16 s, mxos_u16 n, MXOS_FILE *f) { return elements(f,b,s,n,1); }
mxos_u16 mxos_fwrite(const void *b, mxos_u16 s, mxos_u16 n, MXOS_FILE *f) { return elements(f,(void *)b,s,n,2); }
int mxos_fseek(MXOS_FILE *f, mxos_i32 offset, mxos_u8 origin)
{
    mxos_u32 n=(mxos_u32)offset;
    mxos_u8 i;
    for (i=0;i<4;++i) { f->bytes[53+i]=(mxos_u8)n; n>>=8; }
    f->bytes[57]=origin;
    return invoke(f,3);
}
mxos_u32 mxos_ftell(const MXOS_FILE *f)
{
    return (mxos_u32)get16(f->bytes+32) | ((mxos_u32)get16(f->bytes+34)<<16);
}
int mxos_fflush(MXOS_FILE *f) { return invoke(f,4); }
int mxos_fclose(MXOS_FILE *f) { return invoke(f,5); }
int mxos_fgetc(MXOS_FILE *f)
{
    mxos_u8 ch;
    return mxos_read(f,&ch,1)==1 ? ch : MXOS_FILE_EOF;
}
int mxos_fputc(int ch, MXOS_FILE *f)
{
    mxos_u8 byte=(mxos_u8)ch;
    return mxos_write(f,&byte,1)==1 ? byte : MXOS_FILE_EOF;
}
int mxos_feof(const MXOS_FILE *f) { return f->bytes[44]!=0; }
int mxos_ferror(const MXOS_FILE *f) { return f->bytes[43]; }
void mxos_clearerr(MXOS_FILE *f) { f->bytes[43]=0; f->bytes[44]=0; }
