/* Низкоуровневый SDK: вызовы BIOS/DOS и явная консоль, без stdin/stdout/stderr. */
#ifndef MXOS_H
#define MXOS_H
#include "mxos_file.h"

/* Соглашение C-порта: пять плотно расположенных 16-битных слов, sizeof=10. */
typedef struct {
    mxos_u16 entry, af, bc, de, hl;
} MXOS_REGS;
typedef struct { mxos_u8 bytes[32]; } MXOS_DESCRIPTOR;
typedef struct { mxos_u8 bytes[16]; } MXOS_FILE_INFO;
typedef struct { mxos_u8 bytes[48]; } MXOS_DISK_INFO;
typedef struct { mxos_u8 bytes[86]; } MXOS_COPY_REQUEST;
#define MXOS_MAX_PATH 128
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
#define MXOS_BIOS_DIRECTORY 0xC8DE
#define MXOS_BIOS_CLI 0xC8E1
#define MXOS_BIOS_SHELL 0xC8E4
#define MXOS_BIOS_INPUT_HOOK 0xC8E7
#define MXOS_ZF 0x0040
#define MXOS_BIOS_MEMCPY 0xC42D
#define MXOS_BIOS_GETCH 0xC803
#define MXOS_BIOS_PUTCH 0xC809
#define MXOS_BIOS_INPUT 0xC80F
#define MXOS_BIOS_PUTS 0xC818
#define MXOS_BIOS_PRINT_HEX8 0xC815
#define MXOS_BIOS_PRINT_HEX16 0xC82D
#define MXOS_BIOS_KEYSCAN 0xC81B
#define MXOS_BIOS_CURSOR_GET 0xC81E
#define MXOS_BIOS_CURSOR_SET 0xC821
#define MXOS_BIOS_CHECKSUM 0xC82A
#define MXOS_BIOS_MEMTOP_SET 0xC830
#define MXOS_BIOS_MEMTOP_GET 0xC833
#define MXOS_BIOS_FILE_LIST 0xC83F
#define MXOS_BIOS_DRIVE 0xC842
#define MXOS_BIOS_FILE_CREATE 0xC845
#define MXOS_BIOS_FILE_DELETE 0xC84B
#define MXOS_BIOS_FILE_RENAME 0xC84E
#define MXOS_BIOS_FILE_STAT 0xC851
#define MXOS_BIOS_FILE_ATTR 0xC857
#define MXOS_BIOS_FILE_NAME 0xC85A
#define MXOS_BIOS_FILE_LOAD 0xC866
#define MXOS_BIOS_DISK_DRIVER 0xC863
#define MXOS_BIOS_RAMD_DRIVER 0xC86F
#define MXOS_BIOS_PRINT_DEC 0xC86C
#define MXOS_BIOS_UPPER 0xC875
#define MXOS_BIOS_STRCMP 0xC878
#define MXOS_BIOS_STRICMP 0xC87B
#define MXOS_BIOS_FLUSH 0xC899
#define MXOS_BIOS_TIMER_INIT 0xC8A2
#define MXOS_BIOS_TIMER_READ 0xC8A5
#define MXOS_BIOS_UI_DRAW 0xC8A8
#define MXOS_BIOS_UI_INVERSE 0xC8AB
#define MXOS_BIOS_UI_COPY 0xC8B1
#define MXOS_BIOS_ARG_NEXT 0xC8B7
#define MXOS_BIOS_DRIVE_SELECT 0xC8BA
#define MXOS_BIOS_FILE_COPY 0xC8C3
#define MXOS_BIOS_ERROR 0xC8C9
#define MXOS_BIOS_SECTOR 0xC8CF
#define MXOS_BIOS_MODULE_RELEASE 0xC8DB

/* Один мост позволяет обращаться и к остальным уже объявленным ABI. */
void mxos_bios_call(MXOS_REGS *regs) MXOS_FASTCALL;

void *mxos_memcpy(void *destination, const void *source, mxos_u16 bytes);
void *mxos_memmove(void *destination, const void *source, mxos_u16 bytes);
mxos_u16 mxos_checksum(const void *buffer, mxos_u16 bytes);
mxos_u16 mxos_memtop(void);
void mxos_set_memtop(mxos_u16 top) MXOS_FASTCALL;
void mxos_release_module(void);

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
void mxos_timer_init(void);
mxos_u16 mxos_timer_read(void);

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
void mxos_ui_draw(const void *objects, mxos_u8 x_offset);
void mxos_ui_inverse(void *screen, mxos_u8 width, mxos_u8 height);
void mxos_ui_copy(const void *source, void *destination, mxos_u8 width, mxos_u8 height);
#endif
