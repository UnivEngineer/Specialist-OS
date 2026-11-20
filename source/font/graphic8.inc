; A=X mod 4, H=X in two-pixel units, L=cursor Y, C=glyph.
graphicScalar:
    ld b,a
    ld a,h
    add a,4
    ld (bios_vars.cursorX),a
    ld a,l
    sub 7
    ld e,a
    ld a,h
    rrca
    rrca
    and 3Fh
    add a,90h
    ld d,a
    ld l,c
    ld h,FONT_ADDR/256
    ld a,d
    cp 0BFh
    jp z,graphicClipped
    ld a,(bios_vars.inverse)
    or a
    ld a,b
    jp nz,graphicInverse
    dec a
    jp z,graphicNormal2
    dec a
    jp z,graphicNormal4
    jp graphicNormal6
graphicInverse:
    dec a
    jp z,graphicInverse2
    dec a
    jp z,graphicInverse4
    jp graphicInverse6

    IF GRAPH_PRE_MODE
; A=phase index, H=page of the first glyph row for that phase.
graphPrePage:
    add a,a
    add a,a
    add a,a
    IF GRAPH_PRE_MODE == 2
    add a,a
    ENDIF
    ld h,a
    ld a,(fontPreBase+1)
    add a,h
    ld h,a
    ret
    ENDIF

; Rotate once per row and merge both pieces with the screen. The last
; screen column uses a separate clipped loop and never addresses C000h.
    MACRO GRAPHIC_ROWS label, shift, inverse, clipped
label:
    IF GRAPH_PRE_MODE
    ld a,shift/2-1
    call graphPrePage
    ld a,h
    add a,GRAPH_PRE_MODE*8
    ld (.end+1),a
    ENDIF
.row:
    IF GRAPH_PRE_MODE == 2
    ld a,(hl)
    IF inverse
    cpl
    and 255>>shift
    ENDIF
    ld b,a
    ld a,(de)
    and 256-(256>>shift)
    or b
    ld (de),a
    IF !clipped
    inc h
    inc d
    ld a,(hl)
    IF inverse
    cpl
    and 256-(256>>shift)
    ENDIF
    ld b,a
    ld a,(de)
    and 255>>shift
    or b
    ld (de),a
    dec d
    dec h
    ENDIF
    inc h
    ELSE
    ld a,(hl)
    IF inverse
    cpl
    ENDIF
    IF !GRAPH_PRE_MODE
    IF shift == 6
    rlca
    rlca
    ELSE
    DUP shift
    rrca
    EDUP
    ENDIF
    ENDIF
    ld b,a
    ld a,(de)
    xor b
    and 256-(256>>shift)
    xor b
    ld (de),a
    IF !clipped
    inc d
    ld a,(de)
    xor b
    and 255>>shift
    xor b
    ld (de),a
    dec d
    ENDIF
    ENDIF
    inc e
    inc h
    ld a,h
    IF GRAPH_PRE_MODE
.end: cp 0
    ELSE
    cp FONT_ADDR/256+8
    ENDIF
    jp nz,.row
    ret
    ENDM
    GRAPHIC_ROWS graphicNormal2,2,0,0
    GRAPHIC_ROWS graphicNormal4,4,0,0
    GRAPHIC_ROWS graphicNormal6,6,0,0
    GRAPHIC_ROWS graphicInverse2,2,1,0
    GRAPHIC_ROWS graphicInverse4,4,1,0
    GRAPHIC_ROWS graphicInverse6,6,1,0

; The rightmost column clips the spill. Share its cold path across phases,
; using the base bitmap even for a renderer with precomputed shifts.
graphicClipped:
    ld a,b
    add a,a
    ld (graphicClipShift+1),a
    add a,0FEh
    ld c,a
    ld b,0
    ld a,(bios_vars.inverse)
    add a,0FFh
    sbc a,a
    ld (graphicClipInverse+1),a
    push hl
    ld hl,graphicClipMasks
    add hl,bc
    ld a,(hl)
    ld (graphicClipMask+1),a
    pop hl
graphicClipRow:
    ld a,(hl)
graphicClipInverse:
    xor 0
graphicClipShift:
    ld b,0
graphicClipRotate:
    rrca
    dec b
    jp nz,graphicClipRotate
    ld b,a
    ld a,(de)
    xor b
graphicClipMask:
    and 0
    xor b
    ld (de),a
    inc e
    inc h
    ld a,h
    cp FONT_ADDR/256+8
    jp nz,graphicClipRow
    ret
graphicClipMasks: DB 0C0h,0,0F0h,0,0FCh
