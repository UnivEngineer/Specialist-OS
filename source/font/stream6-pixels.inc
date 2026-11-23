    MACRO STREAM6_MERGE keepLeft,inverse
    IF FONT_MODE == 2
    or (hl)
    ELSE
    xor (hl)
    and keepLeft
    xor (hl)
    ENDIF
    IF inverse
    cpl
    ENDIF
    ld (de),a
    inc d
    ENDM

    MACRO STREAM6_ROTATE inverse
    ; C = first code, B = second code. Keep the two parts of each
    ; middle glyph in registers across the neighboring screen bytes.
    pop bc
    ld l,b
    ld a,(hl)
    DUP 4
    rlca
    EDUP
    ld b,a
    and 03h
    ld l,c
    ld c,a
    ld a,(hl)
    add a,a
    add a,a
    or c
    IF inverse
    cpl
    ENDIF
    ld (de),a
    inc d

    ld a,b
    and 0F0h
    pop bc
    ld l,c
    ld c,a
    ld a,(hl)
    rrca
    rrca
    ld l,a
    and 0Fh
    or c
    IF inverse
    cpl
    ENDIF
    ld (de),a
    inc d

    ld a,l
    and 0C0h
    ld c,a
    ld l,b
    ld a,(hl)
    or c
    IF inverse
    cpl
    ENDIF
    ld (de),a
    inc d
    ENDM
