; Пять 16-битных слов: entry, AF, BC, DE, HL; единственный аргумент в HL.
; Мост нереентерабелен: меняется только операнд CALL, пакет хранит вызывающий.
    SECTION code_clib
    INCLUDE "mxos_syscall_ids.inc"
    PUBLIC _mxos_bios_call
    PUBLIC _mxos_call
    PUBLIC _mxos_memmove_far, _mxos_puts_local
    PUBLIC _mxos_printString_bank, _mxos_printString_main
    PUBLIC _mxos_memcpy_far
    PUBLIC _mxos_memtop, _mxos_set_memtop
    PUBLIC _mxos_getch, _mxos_getkey, _mxos_keyscan, _mxos_putch, _mxos_puts
    PUBLIC _mxos_cls, _mxos_cursor_get, _mxos_cursor_set, _mxos_upper

; Консоль/таймер используют общий ABI и одинаково работают в main и BANK.
_mxos_call:
    ld c,(hl)
    inc hl
    ld b,(hl)
    inc hl
    ex de,hl
    call 0F800h
    push af
    pop hl
    ret
_mxos_memmove_far:
    xor a
    call 0F815h
    push af
    pop hl
    ret
_mxos_printString_bank:
_mxos_puts_local:
    ld a,5
    jp 0F815h
; Явный адрес строки main; диспетчер не копирует текст из банка клиента.
_mxos_printString_main:
    call 0F806h
    defw MXOS_BIOS_PRINT_STRING_MAIN
    ret
_mxos_memcpy_far:
    call 0F80Fh
    push af
    pop hl
    ret
_mxos_putch:
    ld c,l
    call 0F806h
    defw MXOS_BIOS_PRINT_CHAR
    ret
_mxos_puts:
    call 0F806h
    defw MXOS_BIOS_PRINT_STRING_BANK
    ret
_mxos_getch:
    call 0F806h
    defw MXOS_BIOS_GETCH
    ld l,a
    ld h,0
    ret
_mxos_getkey:
    call 0F806h
    defw MXOS_BIOS_GETCH
    push af
    pop hl
    ret
_mxos_keyscan:
    call 0F806h
    defw MXOS_BIOS_KEY_SCAN
    ld l,a
    ld h,0
    inc a
    ret nz
    dec h
    ret
_mxos_cls:
    ld c,31
    call 0F806h
    defw MXOS_BIOS_PRINT_CHAR
    ret
_mxos_memtop:
    jp 0C833h
_mxos_set_memtop:
    jp 0C830h
_mxos_cursor_get:
    call 0F806h
    defw MXOS_BIOS_GET_CURSOR_POS
    ret
_mxos_cursor_set:
    call 0F806h
    defw MXOS_BIOS_SET_CURSOR_POS
    ret
_mxos_upper:
    ld a,l
    call 0F806h
    defw MXOS_BIOS_UPPERCASE
    ld l,a
    ld h,0
    ret
_mxos_bios_call:
    push hl
    ld e,(hl)
    inc hl
    ld d,(hl)
    ex de,hl
    ld (bios_target+1),hl
    ex de,hl
    inc hl
    ld e,(hl)
    inc hl
    ld d,(hl)
    push de
    inc hl
    ld c,(hl)
    inc hl
    ld b,(hl)
    inc hl
    ld e,(hl)
    inc hl
    ld d,(hl)
    inc hl
    ld a,(hl)
    inc hl
    ld h,(hl)
    ld l,a
    pop af
bios_target:
    call 0
    push af
    push hl
    push de
    push bc
    ld hl,8
    add hl,sp
    ld e,(hl)
    inc hl
    ld d,(hl)
    ex de,hl
    ld bc,4
    add hl,bc
    pop de
    ld (hl),e
    inc hl
    ld (hl),d
    inc hl
    pop de
    ld (hl),e
    inc hl
    ld (hl),d
    inc hl
    pop de
    ld (hl),e
    inc hl
    ld (hl),d
    pop de
    pop hl
    inc hl
    inc hl
    ld (hl),e
    inc hl
    ld (hl),d
    ret
