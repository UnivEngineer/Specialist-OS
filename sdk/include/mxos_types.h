/* Типы целевого компилятора и соглашение передачи одного аргумента. */
#ifndef MXOS_TYPES_H
#define MXOS_TYPES_H
typedef unsigned char  mxos_u8;
typedef unsigned short mxos_u16;
typedef unsigned long  mxos_u32;
typedef signed   long  mxos_i32;

/* Флаги в младшем байте слова AF, возвращаемого системными вызовами. */
#define MXOS_CF 0x0001
#define MXOS_ZF 0x0040

#ifdef __Z88DK
#define MXOS_FASTCALL __z88dk_fastcall
#else
#define MXOS_FASTCALL
#endif
#endif
