; Fast byte-aligned rectangles; HL=VRAM start, B=width in SCREEN BYTES,
; C=height in pixels. A=fill pattern. B/C=0 is a no-op, otherwise both
; rectangles must fit VRAM without Y wrapping. AF/BC/DE/HL destroyed.
; Caller sets IO_COLOR. uiFrameRectBytes draws a 1px outline, interior intact.
; Unlike the pixel API, top/bottom rows and fills need no edge-mask setup.
uiFillRectBytes:
    ld d,a
    ld a,b
    or a
    ret z
    ld a,c
    or a
    ret z
    ld e,c
uiFillRectByteColumn:
    push hl
    ld c,e
    ld a,d
    call uiFillColumn
    pop hl
    inc h
    dec b
    jp nz,uiFillRectByteColumn
    ret
uiFrameRectBytes:
    ld d,a
    ld a,b
    or a
    ret z
    ld a,c
    or a
    ret z
    ld e,c
    push hl
    push bc
    ld a,d
    call uiRectByteRow
    pop bc
    pop hl
    ld a,e
    cp 1
    ret z
    ld a,l
    add a,e
    dec a
    ld l,a
    push hl
    push bc
    ld a,d
    call uiRectByteRow
    pop bc
    pop hl
    ld a,e
    cp 3
    ret c
    ld a,l
    sub e
    add a,2
    ld l,a
    ld c,e
    dec c
    dec c
    push hl
    push bc
    ld a,d
    and 080h
    ld e,a
uiFrameByteLeft:
    ld a,(hl)
    and 07Fh
    or e
    ld (hl),a
    inc l
    dec c
    jp nz,uiFrameByteLeft
    pop bc
    pop hl
    ld a,b
    dec a
    add a,h
    ld h,a
    ld a,d
    and 1
    ld e,a
uiFrameByteRight:
    ld a,(hl)
    and 0FEh
    or e
    ld (hl),a
    inc l
    dec c
    jp nz,uiFrameByteRight
    ret
uiRectByteRow:
    ld (hl),a
    inc h
    dec b
    jp nz,uiRectByteRow
    ret
