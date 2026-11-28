; This alternative reads the codes eight times but keeps H on a single font
; page and E on a single scanline throughout the horizontal inner loop.
    ld a,(mon2_vars.cursorY)
    sub 7
    ld e,a
    ld h,FONT_ADDR/256
    ld a,(mon2_vars.inverse)
    or a
    jp nz,alignedRowsInverse

    MACRO ALIGNED_SCANLINES label,inverse
label:
    ld a,(graphEnd)
    ld (.limit+1),a
.row:
    push hl
    ld hl,(fast6.text)
    ld b,h
    ld c,l
    pop hl
    ld a,(graphStart)
    ld d,a
.byte:
    ld a,(bc)
    inc bc
    ld l,a
    ld a,(hl)
    IF inverse
    cpl
    ENDIF
    ld (de),a
    inc d
    ld a,d
.limit: cp 0
    jp nz,.byte
    inc e
    inc h
    ld a,h
    cp FONT_ADDR/256+8
    jp nz,.row
    jp graphDone
    ENDM

    ALIGNED_SCANLINES alignedRowsNormal,0
    ALIGNED_SCANLINES alignedRowsInverse,1
