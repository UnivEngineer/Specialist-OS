#include "mxos.h"
static MXOS_PATH path;
int main(void) {
 MXOS_FILE f; char b[8]; mxos_u16 directory, top=mxos_memtop();
 if (sizeof(f)!=64 || sizeof(mxos_u32)!=4 || sizeof(mxos_u16)!=2 || sizeof(void *)!=2) return 1;
 if (sizeof(MXOS_REGS)!=10 || sizeof(MXOS_PATH_IO)!=40) return 12;
 { MXOS_CALL request;
   if (sizeof(request)!=10) return 31;
   request.operation=MXOS_BIOS_UPPERCASE;
   request.hl=0x1234; request.de=0x5678; request.bc=0x9ABC; request.af=0x6101;
   mxos_call(&request);
   if ((request.af>>8)!='A') return 32;
   request.operation=0xFFFF;
   if ((mxos_call(&request)&0xFF01)!=0x0A01) return 33;
 }
 if (mxos_memcpy(b,"ABCD",5)!=b || b[3]!='D') return 13;
 mxos_memmove(b+1,b,4); if (b[1]!='A' || b[4]!='D') return 14;
 if (mxos_checksum("AB",2)!=0x4183) return 15;
 if (top!=TEST_MEMTOP) return 16;
 mxos_set_memtop(top-256); if (mxos_memtop()!=top-256) return 17; mxos_set_memtop(top);
 mxos_cursor_set(0x1008); if (mxos_cursor_get()!=0x1008) return 18;
 mxos_putch('A'); mxos_puts("B");
 if (mxos_getch()!='Q' || (mxos_getkey() & 0xFF01)!=0x5101 || mxos_keyscan()!=-1) return 19;
 if (mxos_upper('a')!='A') return 20;
 if (mxos_equal_n("ABC","AbC",3,0) || !mxos_equal_n("ABC","AbC",3,1)) return 21;
 if (mxos_drive_select(8)!=7 || mxos_drive_select(1)) return 22;
 if (sizeof(path)!=128 || mxos_directory_get(&directory) || directory) return 28;
 path.bytes[0]='/'; if (mxos_chdir(path.bytes)) return 29;
 if (mxos_directory_set(0) || mxos_directory_get(&directory) || directory) return 30;
 if (mxos_fopen(&f,"CSTDIO.BIN","w+b")) return 2;
 if (mxos_fputc('X',&f)!='X') return 3;
 if (mxos_fwrite("ABCD",2,2,&f)!=2) return 4;
 if (mxos_ftell(&f)!=5) return 5;
 if (mxos_fseek(&f,0,MXOS_SEEK_SET)) return 6;
 if (mxos_fgetc(&f)!='X') return 7;
 if (mxos_fread(b,2,3,&f)!=2) return 8;
 if (b[0]!='A' || b[3]!='D' || !mxos_feof(&f) || mxos_ferror(&f)) return 9;
 mxos_clearerr(&f); if (mxos_feof(&f)) return 10;
 if (mxos_fclose(&f)) return 11;
 if (mxos_file_stat(f.bytes) || f.bytes[28]!=5) return 23;
 if (mxos_file_load(f.bytes,b) || b[0]!='X' || b[4]!='D') return 24;
 f.bytes[28]=3; if (mxos_file_save(f.bytes,"NEW")) return 25;
 if (mxos_file_load(f.bytes,b) || b[0]!='N' || b[2]!='W') return 26;
 if (mxos_file_rename(f.bytes,"RENSTDIOBIN") || mxos_file_delete("RENSTDIOBIN")) return 27;
 return 0;
}
