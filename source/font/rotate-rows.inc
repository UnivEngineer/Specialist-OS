; Rotate one four-glyph block across, then down. The same rotated byte
; supplies both parts of each middle glyph. The four codes are patched
; once per block, leaving HL free for the page-aligned font reads.
    MACRO ROTATE_ROW_BLOCK entry,inverse,up,next,limit
entry:
    call scanGroup
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
    IF up
    ld h,FONT_ADDR/256+7
    ELSE
    ld h,FONT_ADDR/256
    ENDIF
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

    dec d
    dec d
    IF up
    dec e
    dec h
    ld a,h
    cp (FONT_ADDR/256-1)&255
    ELSE
    inc e
    inc h
    ld a,h
    cp FONT_ADDR/256+8
    ENDIF
    jp nz,.row
    ; The last iteration stepped once beyond the glyph. Start the next
    ; block on this edge, two columns farther right.
    IF up
    inc e
    ELSE
    dec e
    ENDIF
    inc d
    inc d
    GROUP_FINISH next,limit
    ENDM

    ROTATE_ROW_BLOCK groupNormalDown,0,0,groupNormalUp,normalDownLimit
    ROTATE_ROW_BLOCK groupNormalUp,0,1,groupNormalDown,normalUpLimit
    ROTATE_ROW_BLOCK groupInverseDown,1,0,groupInverseUp,inverseDownLimit
    ROTATE_ROW_BLOCK groupInverseUp,1,1,groupInverseDown,inverseUpLimit
