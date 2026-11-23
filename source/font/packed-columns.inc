; The three screen columns alternate their vertical direction.
    MACRO COLUMN_STEP up
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
    IFDEF FONT_ROTATE_COL4
    INCLUDE "rotate-columns.inc"
    ELSE
    MACRO COLUMN_ROW offset, mask, inverse, advance, up
    ld a,(bc)
    IF FONT_MODE == 0
    IF offset == 0
    DUP 4
    rrca
    EDUP
    and 3
    ELSE
    IF offset == 1
    rrca
    rrca
    and 0Fh
    ENDIF
    ENDIF
    ld (.other+1),a
    ld a,(hl)
    IF offset == 0
    add a,a
    add a,a
    ELSE
    rrca
    rrca
    IF offset == 1
    rrca
    rrca
    and 0F0h
    ELSE
    and 0C0h
    ENDIF
    ENDIF
.other: or 0
    ELSE
    IF FONT_MODE == 2
    or (hl) ; Five separate planes already contain only their own pixel bits.
    ELSE
    xor (hl)
    and mask
    xor (hl)
    ENDIF
    ENDIF
    IF inverse
    cpl
    ENDIF
    ld (de),a
    IF advance
    COLUMN_STEP up
    ENDIF
    ENDM
    MACRO GLYPH_COLUMN offset, mask, inverse, up, hp, bp
    ld hl,(codes+offset)
    ld c,h
    IF FONT_MODE
hp: ld h,0
bp: ld b,0
    ELSE
    ld h,FONT_ADDR/256+up*7
    ld b,h
    ENDIF
    DUP 7
    COLUMN_ROW offset,mask,inverse,1,up
    EDUP
    COLUMN_ROW offset,mask,inverse,0,up
    ENDM
    ENDIF
groupNormalDown:
    call scanGroup
    GLYPH_COLUMN 0,03h,0,0,pre0Normal,pre1Normal
    inc d
    GLYPH_COLUMN 1,0Fh,0,1,pre2NormalUp,pre3NormalUp
    inc d
    GLYPH_COLUMN 2,3Fh,0,0,pre4Normal,pre5Normal
    GROUP_FINISH groupNormalUp,normalDownLimit
groupNormalUp:
    call scanGroup
    GLYPH_COLUMN 0,03h,0,1,pre0NormalUp,pre1NormalUp
    inc d
    GLYPH_COLUMN 1,0Fh,0,0,pre2Normal,pre3Normal
    inc d
    GLYPH_COLUMN 2,3Fh,0,1,pre4NormalUp,pre5NormalUp
    GROUP_FINISH groupNormalDown,normalUpLimit
groupInverseDown:
    call scanGroup
    GLYPH_COLUMN 0,03h,1,0,pre0,pre1
    inc d
    GLYPH_COLUMN 1,0Fh,1,1,pre2Up,pre3Up
    inc d
    GLYPH_COLUMN 2,3Fh,1,0,pre4,pre5
    GROUP_FINISH groupInverseUp,inverseDownLimit
groupInverseUp:
    call scanGroup
    GLYPH_COLUMN 0,03h,1,1,pre0Up,pre1Up
    inc d
    GLYPH_COLUMN 1,0Fh,1,0,pre2,pre3
    inc d
    GLYPH_COLUMN 2,3Fh,1,1,pre4Up,pre5Up
    GROUP_FINISH groupInverseDown,inverseUpLimit
