/* Двоичный файловый ABI MXOS2; структура не зависит от выравнивания C. */
#ifndef MXOS_FILE_H
#define MXOS_FILE_H
typedef unsigned char mxos_u8;
typedef unsigned short mxos_u16;
typedef unsigned long mxos_u32;
typedef signed long mxos_i32;
typedef struct { mxos_u8 bytes[64]; } MXOS_FILE;
/* Целый файл по пути: 40 байт, рабочий дескриптор принадлежит вызывающему.
 * count — вместимость загрузки / длина сохранения; actual — результат.
 * path и buffer — NEAR-адреса; MXOS_DOS_FILE_LOAD_PATH/FILE_SAVE_PATH через mxos_call. */
typedef struct {
    mxos_u16 path, buffer, count, actual;
    mxos_u8 descriptor[32];
} MXOS_PATH_IO;

/* MXOS_BIOS_FILE_BODY: общий загрузчик main, без разбора контейнера.
 * descriptor — готовая запись выбранного диска; header/tail — байты.
 * trailer=0 пропускает хвост, иначе получает tail байт; actual — тело. */
typedef struct {
    mxos_u8 descriptor[32];
    mxos_u16 buffer;
    mxos_u8 header, tail;
    mxos_u16 trailer, actual;
} MXOS_BODY_IO;

/* Low-level mxos_file_call: bytes[63]=operation, bytes[41]=mode.
 * OPTIONS: bytes[57]=0 query / 1 exchange; word at 51 is flags, returns old flags. */
#define MXOS_FILE_OP_OPTIONS 6
#define MXOS_DOS_IO_VERIFY_FILES 1
#define MXOS_FILE_MODE_VERIFY 32
#define MXOS_FILE_MODE_SCATTER 64
#define MXOS_FH_ALLOC_SEED 60

#define MXOS_SEEK_SET 0
#define MXOS_SEEK_CUR 1
#define MXOS_SEEK_END 2
#define MXOS_FILE_EOF (-1)
#define MXOS_ERR_IO 1
#define MXOS_ERR_NOT_FOUND 2
#define MXOS_ERR_EXISTS 3
#define MXOS_ERR_FULL 4
#define MXOS_ERR_DIR_FULL 5
#define MXOS_ERR_ACCESS 6
#define MXOS_ERR_ARGUMENT 7
#define MXOS_ERR_CORRUPT 8
#define MXOS_ERR_NO_DRIVER 9
#define MXOS_ERR_UNSUPPORTED 10
#define MXOS_ERR_BAD_IMAGE 11

/* Переходники компилятора: один указатель на 64-байтный блок; код DOS=0 при успехе. */
#ifdef __Z88DK
#define MXOS_FASTCALL __z88dk_fastcall
#else
#define MXOS_FASTCALL
#endif
mxos_u8 mxos_file_call(MXOS_FILE *file) MXOS_FASTCALL;
mxos_u8 mxos_dos_init(void);
mxos_u8 mxos_current_drive(void);

/* Объект выделяет вызывающий; функции не используют malloc и скрытые буферы. */
int mxos_fopen(MXOS_FILE *file, const char *path, const char *mode);
mxos_u16 mxos_read(MXOS_FILE *file, void *buffer, mxos_u16 bytes);
mxos_u16 mxos_write(MXOS_FILE *file, const void *buffer, mxos_u16 bytes);
mxos_u16 mxos_fread(void *buffer, mxos_u16 size, mxos_u16 count, MXOS_FILE *file);
mxos_u16 mxos_fwrite(const void *buffer, mxos_u16 size, mxos_u16 count, MXOS_FILE *file);
int mxos_fseek(MXOS_FILE *file, mxos_i32 offset, mxos_u8 origin);
mxos_u32 mxos_ftell(const MXOS_FILE *file);
int mxos_fflush(MXOS_FILE *file);
int mxos_fclose(MXOS_FILE *file);
int mxos_fgetc(MXOS_FILE *file);
int mxos_fputc(int ch, MXOS_FILE *file);
int mxos_feof(const MXOS_FILE *file);
int mxos_ferror(const MXOS_FILE *file);
void mxos_clearerr(MXOS_FILE *file);
#endif
