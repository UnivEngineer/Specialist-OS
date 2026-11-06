; Пять 16-битных слов: entry, AF, BC, DE, HL; единственный аргумент в HL.
; Мост нереентерабелен: меняется только операнд CALL, пакет хранит вызывающий.
    SECTION code_clib
    PUBLIC _mxos_bios_call
    PUBLIC _mxos_call
    PUBLIC _mxos_memtop, _mxos_set_memtop, _mxos_release_module
    PUBLIC _mxos_getch, _mxos_getkey, _mxos_keyscan, _mxos_putch, _mxos_puts
    PUBLIC _mxos_cls, _mxos_cursor_get, _mxos_cursor_set, _mxos_upper
    PUBLIC _mxos_timer_init, _mxos_timer_read

; Горячие вызовы без упаковки регистров: putch — 4 байта/15 тактов до BIOS,
; getch — 7 байт, 39 тактов обвязки помимо времени самой клавиатуры.
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
_mxos_putch:
    ld c,l
    jp 0C809h
_mxos_puts:
    jp 0C818h
_mxos_getch:
    call 0C803h
    ld l,a
    ld h,0
    ret
_mxos_getkey:
    call 0C803h
    push af
    pop hl
    ret
_mxos_keyscan:
    call 0C81Bh
    ld l,a
    ld h,0
    inc a
    ret nz
    dec h
    ret
_mxos_cls:
    ld c,31
    jp 0C809h
_mxos_memtop:
    jp 0C833h
_mxos_set_memtop:
    jp 0C830h
_mxos_cursor_get:
    jp 0C81Eh
_mxos_cursor_set:
    jp 0C821h
_mxos_release_module:
    jp 0C8DBh
_mxos_upper:
    ld a,l
    call 0C875h
    ld l,a
    ld h,0
    ret
_mxos_timer_init:
    jp 0C8A2h
_mxos_timer_read:
    jp 0C8A5h

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
