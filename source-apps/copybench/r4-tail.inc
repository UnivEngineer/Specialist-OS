; Одинаковые хвосты страниц. Как R4: SP читает источник, BC/DE несут
; данные, HL держит конец четверки назначения. Блоки 256, затем остаток.
; CALL/RET запрещены до восстановления стека инициатора.
bankCopyTail:
bankCopyRead4:
    pop bc
    pop de
    jp BANK_COPY_DST_GATE
bankCopyWrite4:
    ld sp,hl
    push de
    push bc
    inc hl
    inc hl
    inc hl
    inc hl
    ld a,(BANK_COPY_SOURCE)
    add a,4
    ld (BANK_COPY_SOURCE),a
    jp BANK_COPY_CHECK
bankCopyFastCheck:
    ; Выровненный полный сектор: завершение по переполнению, как в R4.
    jp nz,BANK_WINDOW
    ld a,(BANK_COPY_SOURCE+1)
    inc a
    ld (BANK_COPY_SOURCE+1),a
    jp bankCopyBlock
bankCopyNoCarryCheck:
    jp nc,bankCopyNoCarry
    ld a,(BANK_COPY_SOURCE+1)
    inc a
    ld (BANK_COPY_SOURCE+1),a
bankCopyNoCarry:
    ld a,(BANK_COPY_GROUPS)
    dec a
    ld (BANK_COPY_GROUPS),a
    jp nz,BANK_WINDOW
bankCopyBlock:
    ld a,(BANK_COPY_BLOCKS)
    or a
    jp z,bankCopyLastQuads
    dec a
    ld (BANK_COPY_BLOCKS),a
    ld a,64
    jp bankCopyStartGroup
bankCopyLastQuads:
    ex de,hl
    ld hl,bankCopyNoCarryCheck
    ld (BANK_COPY_CHECK+1),hl
    ex de,hl
    ld a,(BANK_COPY_QUADS)
    or a
    jp z,bankCopyBytesMode
    ld b,a
    xor a
    ld (BANK_COPY_QUADS),a
    ld a,b
bankCopyStartGroup:
    ld (BANK_COPY_GROUPS),a
    jp BANK_WINDOW
bankCopyBytesMode:
    ld a,(BANK_COPY_REST)
    or a
    jp z,bankCopyFinish
    dec hl
    dec hl
    dec hl
    dec hl
    ld de,bankCopyReadByte
    ex de,hl
    ld (BANK_WINDOW+9),hl
    ld hl,bankCopyWriteByte
    ld (BANK_WINDOW+17),hl
    ex de,hl
    jp BANK_WINDOW
bankCopyReadByte:
    ex de,hl
    ld hl,(BANK_COPY_SOURCE)
    ld c,(hl)
    ex de,hl
    jp BANK_COPY_DST_GATE
bankCopyWriteByte:
    ld (hl),c
    inc hl
    ld a,(BANK_COPY_SOURCE)
    inc a
    ld (BANK_COPY_SOURCE),a
    jp nz,bankCopyByteNoCarry
    ld a,(BANK_COPY_SOURCE+1)
    inc a
    ld (BANK_COPY_SOURCE+1),a
bankCopyByteNoCarry:
    ld a,(BANK_COPY_REST)
    dec a
    ld (BANK_COPY_REST),a
    jp nz,BANK_WINDOW
bankCopyFinish:
    ld hl,bankCopyReturn
    ld (BANK_WINDOW+9),hl
    ld a,(BANK_COPY_RETURN_PORT)
    ld (BANK_WINDOW+6),a
    ld a,(BANK_COPY_CALLER)
    ld (BANK_WINDOW+4),a
    jp BANK_WINDOW
bankCopyTailEnd:
