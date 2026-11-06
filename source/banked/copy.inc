; Привилегированный транспорт загрузчика/драйвера, не публичный malloc API.
; HL -> {srcPage,srcOffset:word,dstPage,dstOffset:word,count:word}.
; Пакет находится в текущей странице. Диапазоны заканчиваются до FF00h;
; пересечение диапазонов одной страницы запрещено контрактом memcpy.
; AF=статус, CF=ошибка; BC/DE/HL, страница, SP и все 32 байта окна сохранены.
; Нереентерабельная атомарная петля: без callback и с запрещёнными IRQ.
bankCopyFar:
    push bc
    push de
    push hl
    ld de,bankCopyRequest
    ld b,8
    call bankCopyBytes
    ld hl,(bankCopyRequest+6)
    ld a,h
    or l
    jp z,bankCopyOK
    ld a,(bankCopyRequest)
    call bankCopyPageCheck
    jp c,bankCopyError
    ld a,(bankCopyRequest+3)
    call bankCopyPageCheck
    jp c,bankCopyError
    ld hl,(bankCopyRequest+1)
    call bankCopyRange
    jp c,bankCopyError
    ld hl,(bankCopyRequest+4)
    call bankCopyRange
    jp c,bankCopyError
    ; Сохраняем стандартное окно на время атомарного переноса.
    ld hl,BANK_WINDOW
    ld de,bankCopySavedWindow
    ld b,32
    call bankCopyBytes
    ld hl,bankCopyWindow
    ld de,BANK_WINDOW
    ld b,19
    call bankCopyBytes
    ld hl,(bankCopyRequest+1)
    ld (BANK_COPY_SOURCE),hl
    ld hl,0
    add hl,sp
    ld (BANK_COPY_SP),hl
    ld a,(bankPage)
    call bankCopySelector
    ld (BANK_COPY_RETURN_PORT),a
    ld a,c
    ld (BANK_COPY_CALLER),a
    ld a,0C3h
    ld (BANK_COPY_CHECK),a
    ld hl,bankCopyNoCarryCheck
    ld a,(bankCopyRequest+1)
    or a
    jp nz,bankCopyCheckReady
    ld hl,bankCopyFastCheck
bankCopyCheckReady:
    ld (BANK_COPY_CHECK+1),hl
    ld a,(bankCopyRequest)
    call bankCopySelector
    ld (BANK_WINDOW+6),a
    ld a,c
    ld (BANK_WINDOW+4),a
    ld a,(bankCopyRequest+3)
    call bankCopySelector
    ld (BANK_WINDOW+14),a
    ld a,c
    ld (BANK_WINDOW+12),a
    ld hl,(bankCopyRequest+6)
    ld a,h
    ld (BANK_COPY_BLOCKS),a
    ld a,l
    and 3
    ld (BANK_COPY_REST),a
    ld a,l
    rrca
    rrca
    and 03Fh
    ld (BANK_COPY_QUADS),a
    ld hl,(bankCopyRequest+4)
    inc hl
    inc hl
    inc hl
    inc hl
    jp bankCopyBlock

bankCopyReturn:
    ; Окно пока содержит указатель стека инициатора. Восстанавливаем SP
    ; до первого CALL; перенос шаблона происходит вне самого окна.
    ld hl,(BANK_COPY_SP)
    ld sp,hl
    ld hl,bankCopySavedWindow
    ld de,BANK_WINDOW
    ld b,32
    call bankCopyBytes
bankCopyOK:
    xor a
bankCopyError:
    pop hl
    pop de
    pop bc
    ret

bankCopyPageCheck:
    ld c,a
    ld a,(bankPageCount)
    cp c
    ret nc
    ld a,BANK_ERROR_PAGE
    scf
    ret

; A=логический ID -> A=младший байт порта, C=значение записи.
bankCopySelector:
    ld c,a
    or a
    ld a,0FCh
    ret z
    dec c
    ld a,0FDh
    ret

bankCopyRange:
    push hl
    ld hl,(bankCopyRequest+6)
    ex de,hl
    pop hl
    add hl,de
    jp c,bankCopyBadRange
    ld a,h
    cp 0FFh
    jp c,bankCopyValidRange
    jp nz,bankCopyBadRange
    ld a,l
    or a
    ret z
bankCopyBadRange:
    ld a,BANK_ERROR_RANGE
    scf
    ret
bankCopyValidRange:
    or a
    ret

bankCopyWindow:
    ld sp,0
    ld a,0
    ld (BANK_IO_RAM),a
    jp bankCopyRead4
    ld a,0
    ld (BANK_IO_RAM),a
    jp bankCopyWrite4
bankCopyRequest: DS 8,0
bankCopySavedWindow: DS 32,0
