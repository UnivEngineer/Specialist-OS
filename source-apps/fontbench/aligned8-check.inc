; У 8x8 другая сетка: сверяем каждый байт с тем же charGen, не с сеткой 6x8.
checkAligned8Frame:
    ld a,FONT_ROWS
    ld (aligned8CheckRows),a
    ld de,textCorpus
    ld hl,09000h
checkAligned8Row:
    ld b,48
checkAligned8Char:
    push bc
    push de
    push hl
    ex de,hl
    ld c,(hl)
    ld b,0
    ld hl,(renderFont8)
    add hl,bc
    add hl,hl
    add hl,hl
    add hl,hl
    ld b,8
checkAligned8Byte:
    ld a,(de)
    cp (hl)
    jp nz,checkAligned8Failed
    inc hl
    inc de
    dec b
    jp nz,checkAligned8Byte
    pop hl
    inc h
    pop de
    inc de
    pop bc
    dec b
    jp nz,checkAligned8Char
    ld h,090h
    ld a,l
    add a,10
    ld l,a
    ld a,(aligned8CheckRows)
    dec a
    ld (aligned8CheckRows),a
    jp nz,checkAligned8Row
    ret
checkAligned8Failed:
    pop hl
    pop de
    pop bc
    jp bankMismatch
aligned8CheckRows: DB 0
