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
bankCopyPrepared:
    ; Сохраняем стандартное окно на время атомарного переноса.
    ld hl,BANK_WINDOW
    ld de,bankCopySavedWindow
    ld b,32
    call bankCopyBytes
    ld hl,(bankCopyRequest+1)
    ld (BANK_COPY_SOURCE),hl
    ld hl,(bankCopyRequest+4)
    ld (BANK_COPY_DEST),hl
    ld hl,0
    add hl,sp
    ld (BANK_COPY_SP),hl
    ld a,(bankPage)
    call bankCopyPair
    ld (BANK_COPY_RETURN),hl
    ld a,(bankCopyRequest)
    call bankCopyPair
    ld (BANK_COPY_SRC_SELECT),hl
    ld a,(bankCopyRequest+3)
    call bankCopyPair
    ld (BANK_COPY_DST_SELECT),hl
    ld hl,(bankCopyRequest+6)
    ld a,l
    and 15
    ld (BANK_COPY_REST),a
    ld a,h
    dec a
    or l
    ld bc,16
    jp z,bankCopyStart
    ld b,4
bankCopyDivide16:
    or a
    ld a,h
    rra
    ld h,a
    ld a,l
    rra
    ld l,a
    dec b
    jp nz,bankCopyDivide16
    ld b,h
    ld c,l
bankCopyStart:
    ld de,16
    ld a,b
    or c
    jp nz,bankCopyTail
    jp bankCopyRemainder

; Секторный внутренний вход: тот же пакет, ровно 256 байт.
; Драйвер уже проверил страницы/буфер. Сохраняет регистры, SP и окно,
; поэтому пригоден и для вложенных вызовов из DOS, и из main.
bankCopySector:
    push bc
    push de
    push hl
    ld de,bankCopyRequest
    ld b,8
    call bankCopyBytes
    jp bankCopyPrepared

; HL = пара: L — младший байт регистра маппера, H — физическая страница.
; AF/BC/HL разрушены. Селектор не требует кода в общем окне.
bankCopyPair:
    call bankCopySelector
    ld l,a
    ld h,c
    ret

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

bankCopyRequest: DS 8,0
bankCopySavedWindow: DS 32,0
