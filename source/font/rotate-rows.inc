; Rotate one four-glyph block across, then down. The same rotated byte
; supplies both parts of each middle glyph. The four codes are patched
; once per block, leaving HL free for the page-aligned font reads.
    MACRO ROTATE_ROW_BLOCK entry,inverse
entry:
    ld hl,(codes)
    ld a,l
    ld (.first+1),a
    ld a,h
    ld (.second+1),a
    ld hl,(codes+2)
    ld a,l
    ld (.third+1),a
    ld a,h
    ld (.fourth+1),a
    ld h,FONT_ADDR/256
.row:
    ; A,B,C,D are six-bit glyph rows. The output is A<<2 | B>>4,
    ; B<<4 | C>>2, C<<6 | D. Split each of B and C after one rotation.
.second:
    ld l,0
    ld a,(hl)
    DUP 4
        rlca
    EDUP
    ld b,a
    and 03h
    ld c,a
.first:
    ld l,0
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
    ld c,a
.third:
    ld l,0
    ld a,(hl)
    rrca
    rrca
    ld b,a
    and 0Fh
    or c
    IF inverse
    cpl
    ENDIF
    ld (de),a
    inc d

.fourth:
    ld l,0
    ld c,(hl)
    ld a,b
    and 0C0h
    or c
    IF inverse
    cpl
    ENDIF
    ld (de),a

    inc h
    ld a,h
    cp FONT_ADDR/256+8
    jp z,done              ; DE ends on column 3, row 8, as in the vertical path.
    dec d
    dec d
    inc e
    jp .row
    ENDM

    ROTATE_ROW_BLOCK rotateRowsNormal,0
    ROTATE_ROW_BLOCK rotateRowsInverse,1
