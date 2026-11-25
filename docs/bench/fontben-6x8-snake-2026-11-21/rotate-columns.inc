; One code pair per screen column. HL keeps the unshifted operand.
; Place the two fragments before the last rotation so the merge fits in A.
    MACRO ROTATE_CELL offset,inverse
    ld a,(bc)
    IF offset == 0
    ; Align B[5:4] above the six bits of A, then rotate it into bits 1:0.
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
    ; B[3:0] wraps around C[5:2] before the last right rotation.
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

    MACRO ROTATE_COLUMN_STEP up
    IF up
    dec b
    dec h
    dec e
    ELSE
    inc b
    inc h
    inc e
    ENDIF
    ENDM

    MACRO GLYPH_COLUMN offset,mask,inverse,up,hp,bp
    ld hl,(codes+offset)
    IF offset == 0
    ld c,h
    ELSE
    ld c,l
    ld l,h
    ENDIF
    IF ROTATE_COLUMN_ROWS == 8
    ld h,FONT_ADDR/256+up*7
    ld b,h
    DUP 7
    ROTATE_CELL offset,inverse
    ROTATE_COLUMN_STEP up
    EDUP
    ROTATE_CELL offset,inverse
    ELSE
    ; Down leaves E just past the bottom. Up steps before the store and
    ; consumes that extra row, then leaves E at the top for the next Down.
    ld h,FONT_ADDR/256+up*8
    ld b,h
.rows:
    DUP ROTATE_COLUMN_ROWS
    IF up
    ROTATE_COLUMN_STEP up
    ENDIF
    ROTATE_CELL offset,inverse
    IF !up
    ROTATE_COLUMN_STEP up
    ENDIF
    EDUP
    ld a,h
    cp FONT_ADDR/256+8*(1-up)
    jp nz,.rows
    ENDIF
    ENDM
