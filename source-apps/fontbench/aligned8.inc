; Те же восемь байт глифа charGen, ячейка 8x8, без дополнительных таблиц.
; Пакет сохраняет регистры один раз и не вызывает подпрограмму на каждый глиф.
aligned8Scalar:
    push af
    push bc
    push de
    push hl
    ld a,c
    ld (aligned8Code+1),a
    ld c,1
    call aligned8Validate
    jp c,aligned8Error
    ld hl,aligned8Restore
    ld (aligned8Continue+1),hl
aligned8Code:
    ld a,0
    jp aligned8Glyph
aligned8Batch:
    push af
    push bc
    push de
    push hl
    ld a,c
    or a
    jp z,aligned8Restore
    cp 49
    jp nc,aligned8Error
    push hl
    call aligned8Validate
    pop hl
    jp c,aligned8Error
    ld a,c
    ld (aligned8Remaining),a
    ld (aligned8Read+1),hl
    ld hl,aligned8Next
    ld (aligned8Continue+1),hl
aligned8Read:
    ld hl,0
    ld a,(hl)
    inc hl
    ld (aligned8Read+1),hl
aligned8Glyph:
    ld c,a
    ld b,0
    ld hl,(bios_vars.charGen)
    add hl,bc
    add hl,hl
    add hl,hl
    add hl,hl
    ex de,hl
    ld hl,(bios_vars.cursorY)
    ld a,h
    add a,4
    ld (bios_vars.cursorX),a
    ld a,h
    ; X кратен восьми: два RRCA уже дают номер столбца, AND не нужна.
    rrca
    rrca
    add a,090h
    ld h,a
    ld a,l
    sub 7
    ld l,a
aligned8Pixels:
    DUP 7
    ld a,(de)
    ld (hl),a
    inc e
    inc l
    EDUP
    ld a,(de)
    ld (hl),a
aligned8Continue:
    jp aligned8Restore
aligned8Next:
    ld hl,aligned8Remaining
    dec (hl)
    jp nz,aligned8Read
aligned8Restore:
    pop hl
    pop de
    pop bc
    pop af
    or a
    ret
aligned8Error:
    pop hl
    pop de
    pop bc
    pop af
    scf
    ret
aligned8Validate:
    ld hl,(bios_vars.cursorY)
    ld a,h
    and 3
    jp nz,aligned8Invalid
    ld a,l
    cp 7
    jp c,aligned8Invalid
    ld a,c
    add a,a
    add a,a
    add a,h
    jp c,aligned8Invalid
    cp 193
    jp nc,aligned8Invalid
    or a
    ret
aligned8Invalid:
    scf
    ret
aligned8Remaining: DB 0
