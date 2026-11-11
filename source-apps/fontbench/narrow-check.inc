; Независимое сравнение 6x8 и 4x8 с исходным FONT.FNT и текстом.
; Пиксельные ядра не вызываются, постоянный экранный снимок не нужен.
checkFont6Frame:
    xor a
    jp checkNarrowFrame
checkFont4Frame:
    ld a,1
checkNarrowFrame:
    ld (checkNarrowMode),a
    ld hl,textCorpus
    ld (checkNarrowText),hl
    ld a,FONT_ROWS
    ld (checkNarrowRows),a
    ld bc,09000h
checkNarrowRow:
    ld a,(checkNarrowMode)
    or a
    ld a,12
    jp z,checkNarrowGroups
    ld a,24
checkNarrowGroups:
    ld (checkNarrowLeft),a
checkNarrowGroup:
    push bc
    call checkNarrowCode
    ld (checkNarrowFirst),a
    ld (checkNarrowP0),hl
    call checkNarrowCode
    ld (checkNarrowSecond),a
    ld (checkNarrowP1),hl
    ld a,(checkNarrowMode)
    or a
    jp nz,checkNarrowPointers
    call checkNarrowCode
    ld (checkNarrowP2),hl
    call checkNarrowCode
    ld (checkNarrowP3),hl
checkNarrowPointers:
    pop bc
    ld a,8
    ld (checkNarrowLines),a
checkNarrowLine:
    ld a,(checkNarrowMode)
    or a
    jp z,checkNarrowSix
    ld a,(checkNarrowFirst)
    and 1
    ld hl,(checkNarrowP0)
    ld a,(hl)
    jp z,checkNarrowLeftReady
    rrca
    rrca
    rrca
    rrca
checkNarrowLeftReady:
    and 0F0h
    ld d,a
    ld a,(checkNarrowSecond)
    and 1
    ld hl,(checkNarrowP1)
    ld a,(hl)
    jp nz,checkNarrowRightReady
    rrca
    rrca
    rrca
    rrca
checkNarrowRightReady:
    and 00Fh
    or d
    call checkNarrowCompare
    jp nz,bankMismatch
    jp checkNarrowAdvance
checkNarrowSix:
    ld hl,(checkNarrowP0)
    ld a,(hl)
    add a,a
    add a,a
    ld d,a
    ld hl,(checkNarrowP1)
    ld a,(hl)
    rrca
    rrca
    rrca
    rrca
    and 3
    or d
    call checkNarrowCompare
    jp nz,bankMismatch
    inc b
    ld hl,(checkNarrowP1)
    ld a,(hl)
    rrca
    rrca
    rrca
    rrca
    and 0F0h
    ld d,a
    ld hl,(checkNarrowP2)
    ld a,(hl)
    rrca
    rrca
    and 00Fh
    or d
    call checkNarrowCompare
    jp nz,bankMismatch
    inc b
    ld hl,(checkNarrowP2)
    ld a,(hl)
    rrca
    rrca
    and 0C0h
    ld d,a
    ld hl,(checkNarrowP3)
    ld a,(hl)
    or d
    call checkNarrowCompare
    jp nz,bankMismatch
    dec b
    dec b
checkNarrowAdvance:
    ld hl,checkNarrowP0
    inc (hl)
    inc hl
    inc hl
    inc (hl)
    inc hl
    inc hl
    inc (hl)
    inc hl
    inc hl
    inc (hl)
    inc c
    ld hl,checkNarrowLines
    dec (hl)
    jp nz,checkNarrowLine
    ld a,c
    sub 8
    ld c,a
    inc b
    ld a,(checkNarrowMode)
    or a
    jp nz,checkNarrowNextGroup
    inc b
    inc b
checkNarrowNextGroup:
    ld hl,checkNarrowLeft
    dec (hl)
    jp nz,checkNarrowGroup
    ld b,090h
    ld a,c
    add a,10
    ld c,a
    ld hl,checkNarrowRows
    dec (hl)
    jp nz,checkNarrowRow
    ret
checkNarrowCompare:
    ld e,a
    ld a,(bc)
    cp e
    ret
checkNarrowCode:
    ld hl,(checkNarrowText)
    ld a,(hl)
    inc hl
    ld (checkNarrowText),hl
    ld c,a
    ld a,(checkNarrowMode)
    or a
    ld a,c
    jp z,checkNarrowSixAddress
    or a
    rra
    ld l,a
    ld h,0
    add hl,hl
    add hl,hl
    add hl,hl
    ld de,font4Source
    add hl,de
    ld a,c
    ret
checkNarrowSixAddress:
    ld l,a
    ld h,0
    add hl,hl
    add hl,hl
    add hl,hl
    ld a,h
    add a,028h
    ld h,a
    ld a,c
    ret
checkNarrowFirst: DB 0
checkNarrowSecond: DB 0
checkNarrowText: DW 0
checkNarrowP0: DW 0
checkNarrowP1: DW 0
checkNarrowP2: DW 0
checkNarrowP3: DW 0
checkNarrowMode: DB 0
checkNarrowRows: DB 0
checkNarrowLeft: DB 0
checkNarrowLines: DB 0
