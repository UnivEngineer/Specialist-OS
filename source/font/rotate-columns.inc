; Keep both glyph codes in L/C for the eight rows of one screen column.
; Position the two fragments before the final rotation to merge them in A.
    MACRO ROTATE_COLUMN_BYTE offset,inverse
    ld a,(bc)
    IF offset == 0
    ; Put the right glyph's high two bits above the six left-glyph bits.
    and 30h
    add a,a
    add a,a
    or (hl)
    rlca
    rlca
    ELSE
    rrca
    rrca
    IF offset == 1
    ; The left glyph wraps around the four middle bits of the right glyph.
    xor (hl)
    and 0C3h
    xor (hl)
    rrca
    rrca
    ELSE
    and 0C0h
    or (hl)
    ENDIF
    ENDIF
    IF inverse
    cpl
    ENDIF
    ld (de),a
    ENDM

    MACRO GLYPH_COLUMN offset,mask,inverse,up,hp,bp
    ld hl,(codes+offset)
    IF offset == 0
    ld c,h
    ELSE
    ld c,l
    ld l,h
    ENDIF
    ; Down steps after writing and leaves E below the last row. Up steps
    ; before writing and leaves E on the top row for the next column.
    ld h,FONT_ADDR/256+up*8
    ld b,h
.rows:
    DUP 4
    IF up
    COLUMN_STEP up
    ENDIF
    ROTATE_COLUMN_BYTE offset,inverse
    IF !up
    COLUMN_STEP up
    ENDIF
    EDUP
    ld a,h
    cp FONT_ADDR/256+8*(1-up)
    jp nz,.rows
    ENDM
