; Three page-aligned planes, eight 256-code row pages in each. For a
; six-bit source b: P0=(b<<2)|(b>>4), P1=(b<<4)|(b>>2), P2=(b<<6)|b.
; All operations are byte-wide. P2 includes the original font in bits 0..5;
; ON needs no separate 2 KiB source copy. Generation reads the BIOS font.
    MODULE triple
P0 = PAGED_PRE_BASE
P1 = P0+0800h
P2 = P0+1000h
build:
    ld hl,FONT_ADDR
    ld de,P0
buildByte:
    ld a,(hl)
    and 3Fh
    ld b,a
    DUP 4
        rrca
    EDUP
    and 3
    ld c,a
    ld a,b
    add a,a
    add a,a
    or c
    ld (de),a
    ld a,d
    add a,8
    ld d,a
    ld a,b
    DUP 4
        rrca
    EDUP
    and 0F0h
    ld c,a
    ld a,b
    rrca
    rrca
    and 0Fh
    or c
    ld (de),a
    ld a,d
    add a,8
    ld d,a
    ld a,b
    rrca
    rrca
    and 0C0h
    or b
    ld (de),a
    ld a,d
    sub 16
    ld d,a
    inc de
    inc hl
    ld a,h
    cp (FONT_ADDR+0800h) >> 8
    jp nz,buildByte
    ret

; Select low bits from BC and high bits from HL. XOR-select avoids a
; temporary register/SMC and never reads VRAM. Relative to OR (HL), this
; adds 14 T per output byte on 8080. Each plane has a constant mask.
    MACRO ROW mask, advance
    ld a,(bc)
    xor (hl)
    and mask
    xor (hl)
    ld (de),a
    IF advance
        inc b
        inc h
        inc e
    ENDIF
    ENDM
    MACRO COLUMN offset, plane, mask
    ld hl,(codes+offset)
    ld c,h
    ld h,plane >> 8
    ld b,h
    DUP 7
        ROW mask,1
    EDUP
    ROW mask,0
    ld a,e
    sub 7
    ld e,a
    inc d
    ENDM
; Same packet cursor and text bookkeeping as paged.pre6Helper. Only the
; plane selection and pixel merge differ, permitting a direct comparison.
batch:
    ld a,c
    or a
    ret z
    ld (remaining),a
    ld (text),hl
next:
    ld hl,(text)
    ld a,(hl)
    ld (codes+0),a
    inc hl
    ld a,(hl)
    ld (codes+1),a
    inc hl
    ld a,(hl)
    ld (codes+2),a
    inc hl
    ld a,(hl)
    ld (codes+3),a
    inc hl
    ld (text),hl
    COLUMN 0,P0,03h
    COLUMN 1,P1,0Fh
    COLUMN 2,P2,3Fh
    ld a,(remaining)
    sub 4
    ld (remaining),a
    jp nz,next
    ret

; COLPRE, but HL is now VRAM and DE the packed glyph. B selects glyph
; bits rather than preserved background. The two XORs replace AND/OR:
; (glyph XOR screen) AND mask XOR screen. One extra memory XOR (7 T)
; per output byte; no shifts in the drawing loop. INC D advances font
; rows; INC HL preserves the original COLPRE screen-boundary semantics.
scalar:
    ld a,c
    ld (code+1),a
    ld hl,(bios_vars.cursorY)
    ld a,h
    and 3
    ld (phase+1),a
    ld a,h
    rrca
    rrca
    and 3Fh
    add a,90h
    ld d,a
    ld a,l
    sub 7
    ld e,a
    inc h
    inc h
    inc h
    ld (bios_vars.cursorY),hl
    ex de,hl
code: ld e,0
phase: ld a,0
    or a
    ld d,P0 >> 8
    ld b,0FCh
    jp z,column
    dec a
    ld d,P2 >> 8
    ld b,03Fh
    jp z,column
    dec a
    jp nz,phase6
    ld d,P1 >> 8
    ld b,0Fh
    push hl
    push de
    call column
    pop de
    pop hl
    inc h
    ld d,P2 >> 8
    ld b,0C0h
    jp column
phase6:
    ld d,P0 >> 8
    ld b,3
    push hl
    push de
    call column
    pop de
    pop hl
    inc h
    ld d,P1 >> 8
    ld b,0F0h
column:
    DUP 7
        ld a,(de)
        xor (hl)
        and b
        xor (hl)
        ld (hl),a
        inc d
        inc hl
    EDUP
    ld a,(de)
    xor (hl)
    and b
    xor (hl)
    ld (hl),a
    ret
text: DW 0
codes: DS 4,0
remaining: DB 0
    ENDMODULE
