; CPM32: 32 байта данных, параметры в обоих хвостах; BC/DE — адреса.
bankKeyboardSignature:
    DB 0FFh,0FFh,0FFh,0FFh
bankCopyTail:
    ld hl,(bankCopySourcePair)
    ld a,h
    ld h,0FFh
    ld (hl),a
    ld h,b
    ld l,c
    ld sp,hl
index = 0
    DUP 32/2
    pop hl
    ld (BANK_WINDOW+index*2),hl
index = index+1
    EDUP
    ld hl,0
    add hl,sp
    ld b,h
    ld c,l
    ld hl,(bankCopyDestPair)
    ld a,h
    ld h,0FFh
    ld (hl),a
    ld hl,32
    add hl,de
    ld sp,hl
    ex de,hl
index = 0
    DUP 32/2
    ld hl,(BANK_WINDOW+32-2-index*2)
    push hl
index = index+1
    EDUP
    ld hl,bankCopyCount
    dec (hl)
    jp nz,bankCopyTail
    inc hl
    dec (hl)
    jp nz,bankCopyTail
bankCopySelectHome:
    ld hl,(bankCopyReturnPair)
    ld a,h
    ld h,0FFh
    ld (hl),a
    jp bankCopyReturn
bankCopyLocalParams:
bankCopySourcePair:     DW 0
bankCopyDestPair:       DW 0
bankCopyReturnPair:     DW 0
bankCopyCount:          DW 0
bankCopyTailEnd:
    ASSERT $ <= BANK_WINDOW
