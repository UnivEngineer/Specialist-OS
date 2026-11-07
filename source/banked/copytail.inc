; Одинаковый хвост во всех страницах: CPM16, развёрнутые POP/PUSH.
; Вход BC=число полных порций, DE=16; параметры в общем окне.
; Остаток 0..15 переносится побайтно без чтения за концом диапазона.
; CALL/RET запрещены до возврата в страницу инициатора и восстановления SP.
bankCopyTail:
    ld hl,(BANK_COPY_SRC_SELECT)
    ld a,h
    ld h,0FFh
    ld (hl),a
    ld hl,(BANK_COPY_SOURCE)
    ld sp,hl
    add hl,de
    ld (BANK_COPY_SOURCE),hl
index = 0
    DUP 8
    pop hl
    ld (BANK_WINDOW+index*2),hl
index = index+1
    EDUP
    ld hl,(BANK_COPY_DST_SELECT)
    ld a,h
    ld h,0FFh
    ld (hl),a
    ld hl,(BANK_COPY_DEST)
    add hl,de
    ld (BANK_COPY_DEST),hl
    ld sp,hl
index = 0
    DUP 8
    ld hl,(BANK_WINDOW+14-index*2)
    push hl
index = index+1
    EDUP
    dec bc
    ld a,b
    or c
    jp nz,bankCopyTail
bankCopyRemainder:
    ld a,(BANK_COPY_REST)
    or a
    jp z,bankCopyFinish
    ld b,a
bankCopyByte:
    ld hl,(BANK_COPY_SRC_SELECT)
    ld a,h
    ld h,0FFh
    ld (hl),a
    ld hl,(BANK_COPY_SOURCE)
    ld c,(hl)
    inc hl
    ld (BANK_COPY_SOURCE),hl
    ld hl,(BANK_COPY_DST_SELECT)
    ld a,h
    ld h,0FFh
    ld (hl),a
    ld hl,(BANK_COPY_DEST)
    ld (hl),c
    inc hl
    ld (BANK_COPY_DEST),hl
    dec b
    jp nz,bankCopyByte
bankCopyFinish:
    ld hl,(BANK_COPY_RETURN)
    ld a,h
    ld h,0FFh
    ld (hl),a
    jp bankCopyReturn
bankCopyTailEnd:
