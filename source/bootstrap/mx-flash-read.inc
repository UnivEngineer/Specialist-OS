;----------------------------------------------------------------------------
; MXOS
;
; Чтение 512 байт внешней flash ВВ55 из первых двух гнёзд.
; Вход: DE=LBA 0..1439, HL=буфер текущей страницы.
; Выход: CF=0, BC/DE/HL сохранены, AF изменены.
;----------------------------------------------------------------------------
mxFlashRead:
    push bc
    push de
    push hl
    ex de,hl
    add hl,hl
    ex de,hl
    ld b,2
.block:
    push bc
    ld a,090h
    ld (IO_PROG_MODE),a
    ld a,d
    or 040h
    ld (IO_PROG_C),a
    ld a,00Bh
    ld (IO_PROG_MODE),a
    ld a,e
    ld (IO_PROG_B),a
    ld a,00Ah
    ld (IO_PROG_MODE),a
    ld c,0
.byte:
    ld a,c
    ld (IO_PROG_B),a
    ld a,(IO_PROG_A)
    ld (hl),a
    inc hl
    inc c
    jp nz,.byte
    ld a,0C0h
    ld (IO_PROG_C),a
    inc de
    pop bc
    dec b
    jp nz,.block
    ld a,09Bh
    ld (IO_PROG_MODE),a
    pop hl
    pop de
    pop bc
    xor a
    ret
