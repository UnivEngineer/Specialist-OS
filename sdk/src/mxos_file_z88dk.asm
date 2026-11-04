; Мост sccz80: единственный аргумент __z88dk_fastcall находится в HL.
; Возвращаем код DOS в HL, без зависимости от флагов в вызывающем C.
    SECTION code_clib
    PUBLIC _mxos_file_call
    PUBLIC _mxos_dos_init
    PUBLIC _mxos_current_drive
_mxos_file_call:
    push hl
    ld de,63
    add hl,de
    ld a,(hl)
    pop hl
    call 0C8D5h
    ld l,a
    ld h,0
    ret
_mxos_dos_init:
    call 0C8D8h
    ld l,a
    ld h,0
    ret
_mxos_current_drive:
    ld de,0
    call 0C842h
    ld l,a
    ld h,0
    ret
