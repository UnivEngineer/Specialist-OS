; Временный цикл в резерве C000h страницы DOS.
    INCLUDE "../../source/include/mxos.inc"
    ORG 0C000h
entry:
    IF FONT_TEST
    ld sp,0C200h
    ld de,24
    ld hl,00007h
row:
    ld a,1
    call 0FFCCh
    push hl
    ld b,48
    ld c,' '
glyph:
    xor a
    call 0FFCCh
    inc c
    dec b
    jp nz,glyph
    ld a,2
    call 0FFCCh
    pop hl
    ld a,l
    add a,8
    ld l,a
    dec de
    ld a,d
    or e
    jp nz,row
    ELSE
    ld a,c
    ld (mode),a
row:
    ld hl,text
    ld a,(mode)
    or a
    jp nz,string
glyph:
    ld a,(hl)
    or a
    jp z,endRow
    ld c,a
    MXCALL MX_BIOS_PRINTCHAR
    inc hl
    jp glyph
endRow:
    dec de
    ld a,d
    or e
    jp nz,row
    ret
string:
    MXCALL MX_BIOS_PRINTSTRING
    jp endRow
mode: DB 0
    ENDIF
    IF FONT_TEST
    jp 0FFC6h
    ENDIF
    IF !FONT_TEST
text: DB '0123456789 ABCDEFGHIJKLMNOPQRSTUVWXYZ abcdefghij',13,10,0
    ENDIF
    ASSERT $ <= 0C180h
    DS 0C200h-$,0
