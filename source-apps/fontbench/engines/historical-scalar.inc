; Pixel mechanisms adapted from source/monitor2/loader.inc C060..C16E
; and source/ramfos/printChar.inc. Font decoding/cursor setup is shared.
; The historical erase + XOR behavior is retained, with all 256 glyphs.
    MODULE historical
prepare:
    ld a,c
    ld (glyphCode),a
    ld l,c
    ld h,0
    add hl,hl
    add hl,hl
    add hl,hl
    ld de,02800h
    add hl,de
    ld (glyph),hl
    ld hl,(bios_vars.cursorY)
    ld a,h
    add a,3
    ld (bios_vars.cursorX),a
    ld a,h
    and 3
    ld c,a
    ld a,5
    sub c
    ld c,a
    ld a,h
    and 0FCh
    rrca
    rrca
    add a,90h
    ld h,a
    ld (screen),hl
    ret
monitor2:
    call prepare
    push bc
    ld a,07Fh
    ld (current+1),a
    ld de,solid
    call stackPixels
    pop bc
    ld a,(glyphCode)
    cp 32
    ret z
    xor a
    ld (current+1),a
    ld hl,(glyph)
    ex de,hl
    jp stackPixels
stackPixels:
    ; Preserve the return address beneath sixteen bytes of shifted rows.
    ld b,8
stackRead:
    ld a,(de)
    ld l,a
    ld h,0
    ld a,c
stackShift:
    add hl,hl
    add hl,hl
    dec a
    jp nz,stackShift
    push hl
    inc de
    dec b
    jp nz,stackRead
    ld b,8
    ld hl,(screen)
stackWrite:
    pop de
    ld a,d
    call combine
    ld (hl),a
    inc h
    ld a,e
    call combine
    ld (hl),a
    dec h
    dec l
    dec b
    jp nz,stackWrite
    ret
combine:
    ld c,a
current: ld a,0
    cp 07Fh
    ld a,c
    jp z,eraseByte
    xor (hl)
    ret
eraseByte:
    ld c,a
    ld a,(bios_vars.inverse)
    or a
    jp nz,eraseWhite
    ld a,c
    cpl
    and (hl)
    ret
eraseWhite:
    ld a,(hl)
    or c
    ret
ramfos:
    call prepare
    ex de,hl
    ld hl,03Fh
    ld a,c
maskShift:
    add hl,hl
    add hl,hl
    dec a
    jp nz,maskShift
    push bc
    ld b,8
    call clearChar
    pop bc
    ld a,(glyphCode)
    cp 32
    ret z
    ld hl,(glyph)
    ex de,hl
    ld hl,(screen)
    ld a,l
    sub 7
    ld l,a
    ex de,hl
    jp drawChar
clearChar:
    ld a,(bios_vars.inverse)
    or a
    jp nz,clearWhite
    ld a,h
    cpl
    ld h,a
    ld a,l
    cpl
    ld l,a
clearBlack:
    ld a,(de)
    and h
    ld (de),a
    inc d
    ld a,(de)
    and l
    ld (de),a
    dec d
    dec e
    dec b
    jp nz,clearBlack
    ret
clearWhite:
    ld a,(de)
    or h
    ld (de),a
    inc d
    ld a,(de)
    or l
    ld (de),a
    dec d
    dec e
    dec b
    jp nz,clearWhite
    ret
drawChar:
    ld b,8
drawRow:
    push hl
    ld l,(hl)
    ld h,0
    ld a,c
drawShift:
    add hl,hl
    add hl,hl
    dec a
    jp nz,drawShift
    ld a,(de)
    xor h
    ld (de),a
    inc d
    ld a,(de)
    xor l
    ld (de),a
    dec d
    inc e
    pop hl
    inc hl
    dec b
    jp nz,drawRow
    ret
glyphCode: DB 0
glyph: DW 0
screen: DW 0
solid: DS 8,03Fh
    ENDMODULE
