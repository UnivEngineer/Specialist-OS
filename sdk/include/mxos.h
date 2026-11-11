/* Низкоуровневый SDK: вызовы BIOS/DOS и явная консоль, без stdin/stdout/stderr. */
#ifndef MXOS_H
#define MXOS_H
#include "mxos_file.h"
#include "mxos_syscall_ids.h"

/* Родной диспетчер: operation=(группа<<8)|номер; порядок пакета HL/DE/BC/AF. */
typedef struct {
    mxos_u16 operation, hl, de, bc, af;
} MXOS_CALL;
mxos_u16 mxos_call(MXOS_CALL *request) MXOS_FASTCALL; /* AF результата, включая отказ диспетчера. */

/* Соглашение C-порта: пять плотно расположенных 16-битных слов, sizeof=10. */
typedef struct {
    mxos_u16 entry, af, bc, de, hl;
} MXOS_REGS;
typedef struct { mxos_u8 bytes[32]; } MXOS_DESCRIPTOR;
typedef struct { mxos_u8 bytes[16]; } MXOS_FILE_INFO;
typedef struct { mxos_u8 bytes[48]; } MXOS_DISK_INFO;
typedef struct { mxos_u8 bytes[86]; } MXOS_COPY_REQUEST;
#define MXOS_MAX_PATH 128
#define MXOS_RAW_PREPARE 6 /* RAW_IO B=6: unified entry HL (B=1/2/4/5), DE=0, shift B, drive C. */

/* TEXT_PRINT: near request/source, explicit main buffer for method 2.
 * Z count is an upper bound including NUL; capacity+1 writable bytes required. */
typedef struct {
    mxos_u8 kind, method;
    mxos_u16 source, count, buffer, capacity;
} MXOS_TEXT_PRINT_REQUEST;
#define MXOS_TEXT_PRINT_Z 0
#define MXOS_TEXT_PRINT_RAW_Z 1
#define MXOS_TEXT_PRINT_RAW_LIM 2
#define MXOS_TEXT_PRINT_SHARED 1
#define MXOS_TEXT_PRINT_MAIN_BUFFER 2
#define MXOS_BIOS_ABI_PRINT_CHAR_RAW 0xC869
#define MXOS_BIOS_ABI_PRINT_STRING_Z 0xC818
#define MXOS_BIOS_ABI_PRINT_STRING_RAW_Z 0xC8ED
#define MXOS_BIOS_ABI_PRINT_STRING_RAW_LIM 0xC8F0
#define MXOS_FONT_PRERENDER_BYTES 10240
#define MXOS_FONT_RENDER_QUERY 0
#define MXOS_FONT_RENDER_BUILD 1
#define MXOS_FONT_RENDER_DISABLE 2
#define MXOS_IMAGE_FLAG_DRIVER 0x01
#define MXOS_IMAGE_FLAG_BANK 0x02
#define MXOS_IMAGE_FLAG_MAIN 0x04
#define MXOS_COPY_REPLY_NO 0
#define MXOS_COPY_REPLY_YES 1
#define MXOS_COPY_REPLY_YES_ALL 2
#define MXOS_COPY_REPLY_NO_ALL 3
#define MXOS_CLI_OBSERVER 9
#define MXOS_RAMD_DRIVER_RESTORE 0
#define MXOS_RAMD_DRIVER_BYTE 1
#define MXOS_RAMD_DRIVER_REGISTER 2
#define MXOS_RAMD_DRIVER_BLOCK 3
#define MXOS_RAMD_DRIVER_USER 4
typedef struct { char bytes[MXOS_MAX_PATH]; } MXOS_PATH;

#define MXOS_CF 0x0001
#define MXOS_BIOS_ABI_DIRECTORY 0xC8DE
#define MXOS_BIOS_ABI_CLI 0xC8E1
#define MXOS_BIOS_ABI_SHELL 0xC8E4
#define MXOS_BIOS_ABI_INPUT_HOOK 0xC8E7
#define MXOS_ZF 0x0040
#define MXOS_BIOS_ABI_MEMCPY 0xC42D
#define MXOS_BIOS_ABI_GETCH 0xC803
#define MXOS_BIOS_ABI_PUTCH 0xC809
#define MXOS_BIOS_ABI_INPUT 0xC80F
#define MXOS_BIOS_ABI_PUTS 0xC818
#define MXOS_BIOS_ABI_PUTS_REAL 0xC8ED
#define MXOS_BIOS_ABI_PRINT_HEX8 0xC815
#define MXOS_BIOS_ABI_PRINT_HEX16 0xC82D
#define MXOS_BIOS_ABI_KEY_SCAN 0xC81B
#define MXOS_BIOS_ABI_CURSOR_GET 0xC81E
#define MXOS_BIOS_ABI_CURSOR_SET 0xC821
#define MXOS_BIOS_ABI_CHECKSUM 0xC82A
#define MXOS_BIOS_ABI_MEMTOP_SET 0xC830
#define MXOS_BIOS_ABI_MEMTOP_GET 0xC833
#define MXOS_BIOS_ABI_FILE_LIST 0xC83F
#define MXOS_BIOS_ABI_DRIVE 0xC842
#define MXOS_BIOS_ABI_FILE_CREATE 0xC845
#define MXOS_BIOS_ABI_FILE_DELETE 0xC84B
#define MXOS_BIOS_ABI_FILE_RENAME 0xC84E
#define MXOS_BIOS_ABI_FILE_STAT 0xC851
#define MXOS_BIOS_ABI_FILE_ATTR 0xC857
#define MXOS_BIOS_ABI_FILE_NAME 0xC85A
#define MXOS_BIOS_ABI_FILE_LOAD 0xC866
#define MXOS_BIOS_ABI_DISK_DRIVER 0xC863
#define MXOS_BIOS_ABI_RAMD_DRIVER 0xC86F
#define MXOS_BIOS_ABI_PRINT_DEC 0xC86C
#define MXOS_BIOS_ABI_UPPER 0xC875
#define MXOS_BIOS_ABI_STRCMP 0xC878
#define MXOS_BIOS_ABI_STRICMP 0xC87B
#define MXOS_BIOS_ABI_FLUSH 0xC899
#define MXOS_BIOS_ABI_ARG_NEXT 0xC8B7
#define MXOS_BIOS_ABI_DRIVE_SELECT 0xC8BA
#define MXOS_BIOS_ABI_FILE_COPY 0xC8C3
#define MXOS_BIOS_ABI_ERROR 0xC8C9
#define MXOS_BIOS_ABI_SECTOR 0xC8CF

/* Один мост позволяет обращаться и к остальным уже объявленным ABI. */
void mxos_bios_call(MXOS_REGS *regs) MXOS_FASTCALL;

#define MXOS_MEM_ABI_VERSION 0x0101
#define MXOS_MEM_COPY_FAR 0xF80F
#define MXOS_MEM_INFO 0xF812
#define MXOS_MEM_CAP_COPY 1
#define MXOS_MEM_CAP_LAZY_WINDOW 2
#define MXOS_MEM_CAP_MEMORY_OPS 4
#define MXOS_MEM_CAP_MODULES 8
#define MXOS_MEM_CAP_ALLOCATOR 16
#define MXOS_MEM_CAP_DWORD 32
/* FAR-код: 0=main, 1..16=банки ДОЗУ; packed для 8080, sizeof=8. */
typedef struct {
    mxos_u8 source_bank;
    mxos_u16 source_offset;
    mxos_u8 destination_bank;
    mxos_u16 destination_offset, bytes;
} MXOS_FAR_COPY;
/* Возврат AF: CF=ошибка, A=E0h (банк) или E2h (диапазон).
 * Профиль окна после копирования остаётся установленным. */
mxos_u16 mxos_memcpy_far(MXOS_FAR_COPY *request) MXOS_FASTCALL;

#define MXOS_MEM_SERVICE 0xF815
#define MXOS_MEM_GET32 0xF7A8
#define MXOS_MEM_PUT32 0xF7AB
#define MXOS_ALLOC_MAIN 0
#define MXOS_ALLOC_ANY_BANK 255
/* Операции F815h: 0..5; MXOS_DOS_MEMORY: 1..4, 6..10. */
#define MXOS_MEM_OP_MOVE 0
#define MXOS_MEM_OP_GET8 1
#define MXOS_MEM_OP_GET16 2
#define MXOS_MEM_OP_PUT8 3
#define MXOS_MEM_OP_PUT16 4
#define MXOS_MEM_OP_PRINT 5
#define MXOS_MEM_OP_ALLOC 6
#define MXOS_MEM_OP_FREE 7
#define MXOS_MEM_OP_REALLOC 8
#define MXOS_MEM_OP_ARENA 9
#define MXOS_MEM_OP_RAMLIMIT 10
#define MXOS_MEM_OP_FILL 11
#define MXOS_MEM_OP_QUERY 12
#define MXOS_MEM_OP_DISKCLAIM 13
#define MXOS_MEM_OP_RAMDISK 14
#define MXOS_RAMDISK_STATUS 0
#define MXOS_RAMDISK_COMMIT 1
#define MXOS_MEM_ERR_BANK 0xE0
#define MXOS_MEM_ERR_STACK 0xE1
#define MXOS_MEM_ERR_RANGE 0xE2
#define MXOS_MEM_ERR_NOMEM 0xE3
#define MXOS_MEM_ERR_OWNER 0xE4
#define MXOS_MEM_ERR_HANDLE 0xE5
#define MXOS_MEM_ERR_ACTIVE 0xE6
typedef struct { mxos_u8 bank; mxos_u16 offset; } MXOS_FAR_ADDRESS;
mxos_u16 mxos_memmove_far(MXOS_FAR_COPY *request) MXOS_FASTCALL;
void mxos_puts_local(const char *text) MXOS_FASTCALL;
/* Адрес main передаётся явно; банковый указатель не разыменовывается здесь. */
void mxos_printString_bank(const char *text) MXOS_FASTCALL;
void mxos_printString_main(mxos_u16 address) MXOS_FASTCALL;
/* AF результата; при ошибке realloc адрес не меняется. Дополнительно mxos_far.c. */
mxos_u16 mxos_malloc_far(mxos_u8 bank, mxos_u16 bytes, MXOS_FAR_ADDRESS *out);
mxos_u16 mxos_free_far(MXOS_FAR_ADDRESS *address);
mxos_u16 mxos_realloc_far(MXOS_FAR_ADDRESS *address, mxos_u16 bytes);
mxos_u16 mxos_main_arena(void *start, mxos_u16 bytes);
typedef struct { mxos_u16 free_bytes, largest; mxos_u8 free_records; } MXOS_MEMORY_INFO;
mxos_u16 mxos_fill_far(MXOS_FAR_ADDRESS *address, mxos_u8 value, mxos_u16 bytes);
mxos_u16 mxos_calloc_far(mxos_u8 bank, mxos_u16 count, mxos_u16 size, MXOS_FAR_ADDRESS *out);
mxos_u16 mxos_memory_query(mxos_u8 bank, MXOS_MEMORY_INFO *out);
/* state: 0=не закреплён, 1=владелец DOS, 2=исторический модульный владелец. */
mxos_u16 mxos_ramdisk_status(mxos_u8 *pages, mxos_u8 *state);
mxos_u16 mxos_ramdisk_commit(void); /* Только из DRIVER-инициализатора. */
mxos_u16 mxos_resident_retain(mxos_u16 key);
/* Соглашение входа резидента: A=1 RESTORE, A=2 CLOSE; отказ CLOSE сохраняет модуль. */
mxos_u16 mxos_resident_restore(mxos_u16 handle);
mxos_u16 mxos_resident_close(mxos_u16 handle);
mxos_u16 mxos_resident_kill(mxos_u16 handle);
mxos_u16 mxos_resident_find(mxos_u16 key, mxos_u16 *handle);
mxos_u16 mxos_resident_next(mxos_u8 *index, mxos_u16 *handle, mxos_u16 *key, mxos_u8 *bank);
mxos_u16 mxos_get8_far(MXOS_FAR_ADDRESS *address, mxos_u8 *out);
mxos_u16 mxos_get16_far(MXOS_FAR_ADDRESS *address, mxos_u16 *out);
mxos_u16 mxos_put8_far(MXOS_FAR_ADDRESS *address, mxos_u8 value);
mxos_u16 mxos_put16_far(MXOS_FAR_ADDRESS *address, mxos_u16 value);
mxos_u16 mxos_get32_far(MXOS_FAR_ADDRESS *address, mxos_u32 *out);
mxos_u16 mxos_put32_far(MXOS_FAR_ADDRESS *address, mxos_u32 value);

/* Непрозрачный handle; формат локального MX_REGS совпадает с ASM (8 байт). */
#define MXOS_MODULE_MAIN 0
#define MXOS_MODULE_BANK 1
#define MXOS_MODULE_DOS 2
typedef struct { mxos_u16 hl,de,bc,af; } MXOS_MODULE_REGS;
mxos_u16 mxos_module_load(const char *path,mxos_u8 target,mxos_u16 *handle);
mxos_u16 mxos_module_call(mxos_u16 handle,MXOS_MODULE_REGS *regs);
mxos_u16 mxos_module_unload(mxos_u16 handle);

void *mxos_memcpy(void *destination, const void *source, mxos_u16 bytes);
void *mxos_memmove(void *destination, const void *source, mxos_u16 bytes);
mxos_u16 mxos_checksum(const void *buffer, mxos_u16 bytes);
mxos_u16 mxos_memtop(void);
void mxos_set_memtop(mxos_u16 top) MXOS_FASTCALL;

/* Эти функции обращаются к физической консоли BIOS, не к потокам stdio. */
int mxos_getch(void);
mxos_u16 mxos_getkey(void); /* AF: код в старшем байте, модификаторы во флагах. */
int mxos_keyscan(void); /* -1, если нет нажатой клавиши. */
void mxos_putch(mxos_u8 ch) MXOS_FASTCALL;
void mxos_puts(const char *text) MXOS_FASTCALL;
mxos_u16 mxos_readline(char *buffer, mxos_u16 capacity);
void mxos_cls(void);
mxos_u16 mxos_cursor_get(void); /* X/2 в старшем байте, Y в младшем. */
void mxos_cursor_set(mxos_u16 xy) MXOS_FASTCALL;
void mxos_print_dec(mxos_u16 value, mxos_u8 padding);
void mxos_print_hex8(mxos_u8 value);
void mxos_print_hex16(mxos_u16 value);
mxos_u8 mxos_upper(mxos_u8 ch) MXOS_FASTCALL;
int mxos_equal_n(const char *a, const char *b, mxos_u8 bytes, int ignore_case);

/* Дескриптор/имена здесь уже в формате FAT 8.3; числовой код DOS — результат. */
mxos_u8 mxos_drive_select(mxos_u8 drive);
mxos_u8 mxos_volume_flush(void);
/* CHDIR принимает полный буфер MXOS_MAX_PATH; диск выбирается отдельно. */
mxos_u8 mxos_directory_get(mxos_u16 *cluster);
mxos_u8 mxos_directory_set(mxos_u16 cluster);
mxos_u8 mxos_getcwd(char path[MXOS_MAX_PATH]);
mxos_u8 mxos_chdir(const char path[MXOS_MAX_PATH]);
mxos_u8 mxos_mkdir(const char path[MXOS_MAX_PATH]);
mxos_u8 mxos_rmdir(const char path[MXOS_MAX_PATH]);
mxos_u8 mxos_directory_list(MXOS_FILE_INFO *buffer, mxos_u16 capacity, mxos_u16 start, mxos_u16 *total);
mxos_u8 mxos_last_error(void);
const MXOS_DISK_INFO *mxos_disk_info(mxos_u16 *physical_blocks);
mxos_u8 mxos_file_list(MXOS_FILE_INFO *buffer, mxos_u16 capacity, mxos_u16 start, mxos_u16 *total);
mxos_u8 mxos_file_stat(void *descriptor);
mxos_u8 mxos_file_load(void *descriptor, void *buffer);
mxos_u8 mxos_file_save(void *descriptor, const void *buffer);
mxos_u8 mxos_file_delete(const void *name83);
mxos_u8 mxos_file_rename(const void *old83, const void *new83);
mxos_u8 mxos_file_copy(void *request);
mxos_u8 mxos_file_move(void *request);
/* Имя FAT 8.3; DIR сохраняется, изменяемые биты: R=1,H=2,S=4,A=32. */
mxos_u8 mxos_file_getattr(const void *name83, mxos_u8 *attributes);
mxos_u8 mxos_file_setattr(const void *name83, mxos_u8 attributes);
mxos_u8 mxos_sector_io(mxos_u8 operation, mxos_u16 sector, void *buffer);
/* screen выровнен по байту; width в байтах, height в строках. Размеры ненулевые. */
#endif
