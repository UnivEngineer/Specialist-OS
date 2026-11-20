/* Низкоуровневый SDK: вызовы BIOS/DOS и явная консоль, без stdin/stdout/stderr. */
#ifndef MXOS_H
#define MXOS_H
#include "mxos_file.h"
#include "mxos_syscall_ids.h"

/* Direct local keyboard in DOS/EXEC. Keep one initialized state per input stream. */
typedef struct { mxos_u8 last_key, repeat_count, locks, koi7; mxos_u16 column; mxos_u8 row_mask, repeat_start, timed, soft_divider, soft_ticks; } MXOS_KEY_STATE;
#define MXOS_TIMER_RAW 0
#define MXOS_TIMER_CLOCK 1
#define MXOS_TIMER_RELEASE 2
#define MXOS_TIMER_QUERY 3
#define MXOS_KEY_STATE_INIT {255,0,0,0,0,0,0,0,0,0}
#define MXOS_KEY_EVENT 0x0100
#define MXOS_KEY_CTRL  0x0200
#define MXOS_KEY_MOD_SHIFT 1
#define MXOS_KEY_MOD_CTRL  2
/* Begin returns 1 with the interval clock, 0 with software timing. */
mxos_u8 mxos_bank_key_begin(MXOS_KEY_STATE *state);
void mxos_bank_key_end(MXOS_KEY_STATE *state);
/* Instantaneous physical state in DOS/EXEC. Shift returns 0 or 1. */
mxos_u8 mxos_bank_key_shift(void);
mxos_u8 mxos_bank_key_modifiers(void);
/* Result: event flags and character in the low byte, or 0 if idle. */
mxos_u16 mxos_bank_key_poll(MXOS_KEY_STATE *state) MXOS_FASTCALL;
mxos_u16 mxos_bank_getch_cursor(MXOS_KEY_STATE *state) MXOS_FASTCALL;
mxos_u16 mxos_bank_getch(MXOS_KEY_STATE *state) MXOS_FASTCALL;

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
typedef struct {
    mxos_u16 handle;
    mxos_u8 mode,state,answer,error;
    mxos_u32 bytes;
    mxos_u16 files;
    mxos_u8 flags,phase,known,reserved;
    mxos_u32 total;
    mxos_u16 required,path,totalFiles;
    MXOS_COPY_REQUEST request; /* BEGIN input; returned for CONFIRM only. */
} MXOS_FILEOPS_JOB;
#define MXOS_FILEOPS_BEGIN 0
#define MXOS_FILEOPS_STEP 1
#define MXOS_FILEOPS_ANSWER 2
#define MXOS_FILEOPS_CANCEL 3
#define MXOS_FILEOPS_CLOSE 4
#define MXOS_FILEOPS_SKIP_PREPARE 5
#define MXOS_FILEOPS_NO_PREPARE 1
#define MXOS_FILEOPS_PHASE_COPY 0
#define MXOS_FILEOPS_PHASE_PREPARE 1
#define MXOS_FILEOPS_PHASE_DELETE 2
#define MXOS_FILEOPS_COPY 0
#define MXOS_FILEOPS_MOVE 1
#define MXOS_FILEOPS_DELETE 2
#define MXOS_FILEOPS_RUNNING 0
#define MXOS_FILEOPS_CONFIRM 1
#define MXOS_FILEOPS_DONE 2
#define MXOS_FILEOPS_FAILED 3
/* CF reports API failure; job.error reports an operation failure. Always CLOSE. */
mxos_u16 mxos_fileops(mxos_u8 operation,MXOS_FILEOPS_JOB *job);
/* Request and output buffers are NEAR pointers in the caller's bank.
 * One cursor, owned by the caller; always CLOSE, even after END/error. */
typedef struct {
    mxos_u16 handle,path,buffer,capacity,count;
    mxos_u8 flags,state;
} MXOS_ENUM_REQUEST;
typedef struct { MXOS_DESCRIPTOR descriptor; char path[128]; } MXOS_ENUM_ENTRY;
#define MXOS_ENUM_OPEN 0
#define MXOS_ENUM_NEXT 1
#define MXOS_ENUM_CLOSE 2
#define MXOS_ENUM_RECURSIVE 1
#define MXOS_ENUM_CHECK_CANCEL 2
mxos_u16 mxos_enumerate(mxos_u8 operation,MXOS_ENUM_REQUEST *request);
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
/* MXOS2 v2 on-disk container, shared by COM/EXE/DLL/FNT. Little-endian.
 * BSS is reserved, not cleared by DOS. C startup initializes its own BSS.
 * mem_begin/end describe one contiguous half-open interval; min_heap is
 * required additional space, not an allocation. No relocation is performed. */
typedef struct {
    mxos_u8 signature[5],flags;
    mxos_u16 load,entry,body_bytes,body_checksum,bss,mem_begin,mem_end,min_heap;
    mxos_u8 policy,api;
    mxos_u16 stack_bytes;
    mxos_u8 mode,header_bytes,options,reserved;
    mxos_u16 header_checksum;
} MXOS_IMAGE_HEADER;
#define MXOS_IMAGE_HEADER_SIZE 32
#define MXOS_IMAGE_VERSION 2
#define MXOS_IMAGE_BSS_UNKNOWN 0xffff
#define MXOS_IMAGE_POLICY_EXCLUSIVE 0
#define MXOS_IMAGE_POLICY_PRESERVE 1
#define MXOS_IMAGE_POLICY_MANAGED 2
#define MXOS_IMAGE_MODE_AUTO 0
#define MXOS_IMAGE_MODE_MX 1
#define MXOS_IMAGE_MODE_STD 2
#define MXOS_IMAGE_MODE_MON 3
#define MXOS_IMAGE_MODE_RF 4
#define MXOS_IMAGE_OPTION_CACHEABLE 1
#define MXOS_IMAGE_OPTION_REUSABLE 2
#define MXOS_MODULE_ACTIVATE 0xF0
/* Trusted FONT installer / BIOS restoration, NEAR 128-byte command buffer. */
#define MXOS_FONT_CONFIG_REGISTER 0
#define MXOS_FONT_CONFIG_PREPARE 1
#define MXOS_FONT_CONFIG_FINISH 2
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
#define MXOS_BIOS_ABI_DISK_DRIVER 0xC863
#define MXOS_BIOS_ABI_RAMD_DRIVER 0xC86F
#define MXOS_BIOS_ABI_PRINT_DEC 0xC86C
#define MXOS_BIOS_ABI_UPPER 0xC875
#define MXOS_BIOS_ABI_STRCMP 0xC878
#define MXOS_BIOS_ABI_STRICMP 0xC87B
#define MXOS_BIOS_ABI_ARG_NEXT 0xC8B7

/* Один мост позволяет обращаться и к остальным уже объявленным ABI. */
void mxos_bios_call(MXOS_REGS *regs) MXOS_FASTCALL;

#define MXOS_MEM_ABI_VERSION 0x0103
#define MXOS_MEM_COPY_FAR 0xF80F
#define MXOS_MEM_INFO 0xF812
#define MXOS_MEM_CAP_COPY 1
#define MXOS_MEM_CAP_LAZY_WINDOW 2
#define MXOS_MEM_CAP_MEMORY_OPS 4
#define MXOS_MEM_CAP_MODULES 8
#define MXOS_MEM_CAP_ALLOCATOR 16
#define MXOS_MEM_CAP_COMMAND_QUEUE 32
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
#define MXOS_MEM_OP_RESIDENT 15
/* HL=start, BC=bytes: evict inactive overlapping MAIN modules; no writes. */
#define MXOS_MEM_OP_MAINCLAIM 16
#define MXOS_MEM_OP_MAINREGISTER 17
#define MXOS_MEM_OP_MAINRETIRE 18
#define MXOS_MEM_RESIDENT_FONT 0
#define MXOS_MEM_RESIDENT_SLOTS 4
#define MXOS_RESIDENT_RETAIN 0
#define MXOS_RESIDENT_FIND 1
#define MXOS_RESIDENT_ENUM 2
#define MXOS_RESIDENT_SHELL_SET 3
#define MXOS_RESIDENT_SHELL_RESUME 4
#define MXOS_RESIDENT_DIRECTORY_EPOCH 5
#define MXOS_RESIDENT_EPOCH_ALL 0xFE
#define MXOS_RESIDENT_RELEASE 6
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
/* Exact owned MAIN buffer; no arena change, allocation/clearing or eviction.
 * Rejects zero base/size and every overlap; release via mxos_free_far. */
mxos_u16 mxos_main_register(void *start, mxos_u16 bytes);
typedef struct { mxos_u16 free_bytes, largest; mxos_u8 free_records; } MXOS_MEMORY_INFO;
mxos_u16 mxos_fill_far(MXOS_FAR_ADDRESS *address, mxos_u8 value, mxos_u16 bytes);
mxos_u16 mxos_calloc_far(mxos_u8 bank, mxos_u16 count, mxos_u16 size, MXOS_FAR_ADDRESS *out);
mxos_u16 mxos_memory_query(mxos_u8 bank, MXOS_MEMORY_INFO *out);
/* state: 0=не закреплён, 1=владелец DOS, 2=исторический модульный владелец. */
mxos_u16 mxos_ramdisk_status(mxos_u8 *pages, mxos_u8 *state, mxos_u8 *fresh);
mxos_u16 mxos_ramdisk_commit(void); /* Только из DRIVER-инициализатора. */
mxos_u16 mxos_resident_retain(mxos_u16 key);
/* Соглашение входа резидента: A=1 RESTORE, A=2 CLOSE; отказ CLOSE сохраняет модуль. */
mxos_u16 mxos_resident_restore(mxos_u16 handle);
mxos_u16 mxos_resident_close(mxos_u16 handle);
mxos_u16 mxos_resident_kill(mxos_u16 handle);
mxos_u16 mxos_resident_find(mxos_u16 key, mxos_u16 *handle);
mxos_u16 mxos_resident_next(mxos_u8 *index, mxos_u16 *handle, mxos_u16 *key, mxos_u8 *bank);
mxos_u16 mxos_shell_resident(mxos_u16 key); /* 0 selects file-based shell loading. */
mxos_u16 mxos_shell_resume(void); /* No saved PC/SP; invokes the resident entry. */
mxos_u16 mxos_directory_epoch(mxos_u16 *low, mxos_u16 *high);
mxos_u16 mxos_drive_epochs(mxos_u8 *epochs); /* 32 bytes: eight LE32 values. */
mxos_u16 mxos_resident_release(void); /* Drop own key; return to OS for unload. */
mxos_u16 mxos_get8_far(MXOS_FAR_ADDRESS *address, mxos_u8 *out);
mxos_u16 mxos_get16_far(MXOS_FAR_ADDRESS *address, mxos_u16 *out);
mxos_u16 mxos_put8_far(MXOS_FAR_ADDRESS *address, mxos_u8 value);
mxos_u16 mxos_put16_far(MXOS_FAR_ADDRESS *address, mxos_u16 value);

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
/* Local queue callback may clobber registers. Flush preserves them all.
 * The application owns callback code and its command buffer. */
void mxos_queue_bind(void (*flush)(void)) MXOS_FASTCALL;
void mxos_queue_flush(void);
#define MXOS_ROUTE_CALL_ADDR 0xF818
#define MXOS_QUEUE_BIOS 0
#define MXOS_QUEUE_DOS 1
#define MXOS_QUEUE_DLL 2
#define MXOS_QUEUE_BIND_ADDR 0xF809
#define MXOS_QUEUE_FLUSH_ADDR 0xF80C
#define MXOS_COMMAND_REG_HL 1
#define MXOS_COMMAND_REG_DE 2
#define MXOS_COMMAND_REG_BC 4
#define MXOS_COMMAND_REG_AF 8
#define MXOS_COMMAND_INLINE_HL 16
#define MXOS_COMMAND_BATCH_MAX 256
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
