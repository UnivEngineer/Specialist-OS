; Disposable compact initializer. ABI and flat input match Init6.dll.
; IMPORT keeps only KOI8 20..7F, C0..FF, A3/B3. GENERATE needs no MAIN data.
    INCLUDE "../include/mxos.inc"
    INCLUDE "../include/font-init.inc"
    ORG FONT_INIT_ADDR
    jp initialize
    DB 'FIN1',1
initialize:
    or a
    jp z,importFlat
    cp FONT_INIT_GENERATE
    jp nz,bad
    xor a
    ret
bad:
    ld a,DOS_ERR_ARGUMENT
    scf
    ret
importFlat:
    ld e,(hl)
    inc hl
    ld d,(hl)
    inc hl
    push de
    ld e,(hl)
    inc hl
    ld d,(hl)
    ex de,hl
    ld (source),hl
    pop hl
    push hl
    ld de,42
    add hl,de
    ld e,(hl)
    inc hl
    ld d,(hl)
    ex de,hl
    ld de,FONT_ENGINE_BASE
    call bios_sub_hl_de
    pop de
    add hl,de
    ex de,hl ; DE=staged compact font, not its resident address.
    ld hl,codes
    ld c,162
nextGlyph:
    ld a,(hl)
    inc hl
    push hl
    push bc
    ld l,a
    ld h,0
    add hl,hl
    add hl,hl
    add hl,hl
    push hl
    ld hl,(source)
    ld b,h
    ld c,l
    pop hl
    add hl,bc
    ld b,8
nextRow:
    ld a,(hl)
    and 63
    ld (de),a
    inc hl
    inc de
    dec b
    jp nz,nextRow
    pop bc
    pop hl
    dec c
    jp nz,nextGlyph
    xor a
    ret
source: DW 0
codes: INCBIN "../../bin/fonts/compact-codes.bin"
    ASSERT $ <= FONT_INIT_END
