; Временный цикл в резерве C000h страницы DOS.
    INCLUDE "../../source/include/mxos.inc"
    ORG 0C000h
entry:
    IF FONT_TEST
    ld sp,0C800h
    ld a,(quadMode)
    or a
    jp z,scalarStart
    cp 2
    jp z,serialStart
    cp 3
    jp nc,windowStart
    jp quadStart
scalarStart:
    ld a,24
    ld (rowsLeft),a
    ld de,0C200h
    ld hl,00007h
row:
    ld a,1
    call 0FFCCh
    push hl
    ld b,48
glyph:
    ld a,(de)
    ld c,a
    xor a
    call 0FFCCh
    inc de
    dec b
    jp nz,glyph
    ld a,2
    call 0FFCCh
    pop hl
    ld a,l
    add a,8
    ld l,a
    ld a,(rowsLeft)
    dec a
    ld (rowsLeft),a
    jp nz,row
    jp 0FFC6h
quadStart:
    ld a,24
    ld (rowsLeft),a
    ld hl,0C200h
    ld (quadText),hl
    ld hl,00007h
quadRow:
    ld a,1
    call 0FFCCh
    push hl
    ld a,12
    ld (groupsLeft),a
quadGroup:
    ld hl,(quadText)
    ld b,(hl)
    inc hl
    ld c,(hl)
    inc hl
    ld d,(hl)
    inc hl
    ld e,(hl)
    inc hl
    ld (quadText),hl
    ld a,3
    call 0FFCCh
    ld a,(groupsLeft)
    dec a
    ld (groupsLeft),a
    jp nz,quadGroup
    ld a,2
    call 0FFCCh
    pop hl
    ld a,l
    add a,8
    ld l,a
    ld a,(rowsLeft)
    dec a
    ld (rowsLeft),a
    jp nz,quadRow
    jp 0FFC6h
serialStart:
    ld a,24
    ld (rowsLeft),a
    ld hl,0C200h
    ld (quadText),hl
    ld hl,00007h
serialRow:
    ld a,1
    call 0FFCCh
    push hl
    ld hl,(quadText)
    ld c,48
    ld a,4
    call 0FFCCh
    ld de,48
    add hl,de
    ld (quadText),hl
    ld a,2
    call 0FFCCh
    pop hl
    ld a,l
    add a,8
    ld l,a
    ld a,(rowsLeft)
    dec a
    ld (rowsLeft),a
    jp nz,serialRow
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
rowsLeft: DB 0
groupsLeft: DB 0
quadText: DW 0
windowFarPacket:
    ld a,1
    call 0FFC0h
    ld h,0
    ld l,c
    add hl,de
    ex de,hl
    ret
; Far48 передаёт всю строку одним копированием; Win20 — порциями 20+20+8.
windowTextRow:
    ld a,(quadMode)
    cp 4
    ld c,48
    jp z,windowFarPacket
    ld b,2
windowPacket20:
    ld c,20
    call windowPacket
    dec b
    jp nz,windowPacket20
    ld c,8
    jp windowPacket
    ENDIF
    IF !FONT_TEST
text: DB '0123456789 ABCDEFGHIJKLMNOPQRSTUVWXYZ abcdefghij',13,10,0
    ENDIF
    ASSERT $ <= 0C180h
    IF FONT_TEST
    DS 0C180h-$,0
quadMode: DB 0
windowStart:
    ld a,24
    ld (rowsLeft),a
    ld de,0C200h
    ld hl,00007h
windowRow:
    ld a,2
    call 0FFC0h
    push hl
    call windowTextRow
    ld a,3
    call 0FFC0h
    pop hl
    ld a,l
    add a,8
    ld l,a
    ld a,(rowsLeft)
    dec a
    ld (rowsLeft),a
    jp nz,windowRow
    xor a
    jp 0FFC0h
windowPacket:
    push bc
    ld hl,0FFCCh
windowCopy:
    ld a,(de)
    ld (hl),a
    inc de
    inc hl
    dec c
    jp nz,windowCopy
    pop bc
    ld a,1
    jp 0FFC0h
    DS 0C200h-$,0
corpus: DS 48*24,0
    ASSERT $ <= 0C700h
    DS 0C7FEh-$,0
    DW 0C000h
    ELSE
    DS 0C200h-$,0
    ENDIF
