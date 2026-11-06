; Автозапуск неизменённого CPM.RKS из MX-страницы ROM.
; До перехода в программу стек не используется. 8000h виден в ROM и RAM.
    ORG 0
    di
    ld hl,0100h
    ld de,0C000h
    ld bc,CPM_BODY_SIZE
copyBody:
    ld a,(hl)
    ld (de),a
    inc hl
    inc de
    dec bc
    ld a,b
    or c
    jp nz,copyBody
    ld hl,gate
    ld de,08000h
    ld b,gateEnd-gate
copyGate:
    ld a,(hl)
    ld (de),a
    inc hl
    inc de
    dec b
    jp nz,copyGate
    jp 08000h
gate:
    ld (0FFFCh),a
    ld hl,0C000h
    ld de,0100h
    ld bc,CPM_BODY_SIZE
gateCopy:
    ld a,(hl)
    ld (de),a
    inc hl
    inc de
    dec bc
    ld a,b
    or c
    jp nz,08000h+gateCopy-gate
    jp 0100h
gateEnd:
    ASSERT 0C000h+CPM_BODY_SIZE <= 0FFC0h
    ASSERT $ <= 0100h
