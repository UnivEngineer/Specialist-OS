; CPM32: 32 байта данных, параметры в обоих хвостах; BC/DE — адреса.
bankKeyboardSignature:
    DB 0FFh,0FFh,0FFh,0FFh
bankCopyTail:
bankCopySourcePair = $+1
    ld hl,0
    ld a,h
    ld h,0FFh
    ld (hl),a
    ld h,b
    ld l,c
    ld sp,hl
; Чтение одной порции сохраняет Z. Для TEXT это признак возврата на локальный стек.
bankCopyRead32:
    ASSERT bankCopyRead32 == 0FF0Eh
index = 0
    DUP 32/2
    pop hl
    ld (BANK_WINDOW+index*2),hl
index = index+1
    EDUP
    jp z,MEM_FORWARD_RESTORE
    ld hl,0
    add hl,sp
    ld b,h
    ld c,l
bankCopyDestPair = $+1
    ld hl,0
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
bankCopyReturnPair = $+1
    ld hl,0
    ld a,h
    ld h,0FFh
    ld (hl),a
    jp bankCopyReturn
bankCopyCount: DW 0
bankCopyTailEnd:
    ASSERT $ <= BANK_WINDOW
