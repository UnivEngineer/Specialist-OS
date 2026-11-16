; Three packed planes; all row loops are expanded at assembly time.
    MACRO PACKED_ROW mask, inverse, advance
    ld a,(bc)
    IF FONT_MODE == 2
    or (hl) ; Five separate planes already contain only their own pixel bits.
    ELSE
    xor (hl)
    and mask
    xor (hl)
    ENDIF
    IF inverse
    cpl
    ENDIF
    ld (de),a
    IF advance
    inc b
    inc h
    inc e
    ENDIF
    ENDM
    MACRO PACKED_COLUMN offset, mask, inverse, last, hp, bp
    ld hl,(codes+offset)
    ld c,h
hp: ld h,0
bp: ld b,0
    DUP 7
    PACKED_ROW mask,inverse,1
    EDUP
    PACKED_ROW mask,inverse,0
    IF !last
    ld a,e
    sub 7
    ld e,a
    inc d
    ENDIF
    ENDM
columnsPackedNormal:
    PACKED_COLUMN 0,03h,0,0,pre0Normal,pre1Normal
    PACKED_COLUMN 1,0Fh,0,0,pre2Normal,pre3Normal
    PACKED_COLUMN 2,3Fh,0,1,pre4Normal,pre5Normal
    jp done
columnsPackedInverse:
    PACKED_COLUMN 0,03h,1,0,pre0,pre1
    PACKED_COLUMN 1,0Fh,1,0,pre2,pre3
    PACKED_COLUMN 2,3Fh,1,1,pre4,pre5
    jp done
