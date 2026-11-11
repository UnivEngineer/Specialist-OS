buildLinear:
    ld de,02800h
    ld c,0
linearGlyph:
    ld h,FONT_ADDR >> 8
    ld l,c
    DUP 8
    ld a,(hl)
    ld (de),a
    inc h
    inc de
    EDUP
    inc c
    jp nz,linearGlyph
    ret
buildShifts:
    ld hl,02800h
    ld de,03000h
    ld bc,2048
shiftZero:
    ld a,(hl)
    add a,a
    add a,a
    ld (de),a
    inc hl
    inc de
    dec bc
    ld a,b
    or c
    jp nz,shiftZero
    ld de,04000h
    ld a,2
    call buildPair
    ld de,05000h
    ld a,4
buildPair:
    ld (pairRotations+1),a
    ld hl,02800h
    ld bc,2048
pairByte:
    push bc
    ld a,(hl)
pairRotations: ld b,2
pairRotate:
    rrca
    dec b
    jp nz,pairRotate
    ld c,a
    ld a,(pairRotations+1)
    cp 2
    ld a,c
    jp nz,pairFour
    and 00Fh
    ld (de),a
    inc de
    ld a,c
    and 0C0h
    jp pairWrite
pairFour:
    and 3
    ld (de),a
    inc de
    ld a,c
    and 0F0h
pairWrite:
    ld (de),a
    inc de
    inc hl
    pop bc
    dec bc
    ld a,b
    or c
    jp nz,pairByte
    ret
renderFont8: DW 02800h/8
variant: DB 0
preEnabled: DB 0
