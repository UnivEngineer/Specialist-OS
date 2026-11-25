; Публичное FAR-копирование адресов; не malloc и не проверка владельца.
; HL -> {srcPage,srcOffset:word,dstPage,dstOffset:word,count:word}.
; Пакет находится в текущей странице. Исключающие концы не выше MEM_DATA_LIMIT.
; Копирование внутри одной страницы допускает перекрытие и сохраняет окно.
; Между страницами окно используется для данных. AF=статус, CF=ошибка.
; BC/DE/HL, банк и SP сохранены.
; Нереентерабельная атомарная петля: без callback и с запрещёнными IRQ.

;----------------------------------------------------------------------------
; MXOS
;
; Скопировать байты между страницами по запросу FAR.
;
; вход:
;   hl = локальный адрес восьми байт: страница источника, его смещение, страница
;   назначения, её смещение, длина. Смещения и длина — слова. Диапазоны ограничены
;   MEM_DATA_LIMIT. При совпадении страниц диапазоны могут перекрываться.
;
; выход:
;   cf = 0 при успехе, при отказе cf = 1 и a содержит ошибку MEM. Для нулевой длины
;   копирование пропускается. bc, de, hl, исходная страница и SP сохраняются.
;----------------------------------------------------------------------------

bankCopyFar:
    call bankQueueBefore
bankCopyEnter:
    push bc
    push de
    push hl
    ld de,bankCopyRequest
    ld b,2
    call bankCopyQuads
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
    jp MEM_COPY_SELECT_ADDR
bankCopyPrepared:
    jp MEM_COPY_PREPARE_ENTRY

; Секторный внутренний вход: тот же пакет, ровно 256 байт.
; Драйвер уже проверил страницы/буфер. Регистры и SP сохраняются.
; Вызывающий не держит в общем окне данные после передачи.
bankCopySector:
bankCopyPhysical:
    call bankQueueBefore
    push bc
    push de
    push hl
    ld de,bankCopyRequest
    ld b,2
    call bankCopyQuads
    jp bankCopyPrepared

; A=FAR-код -> HL: L — младший байт маппера, H — физическая страница.
; 80h..8Fh — внутренний ROM-профиль, недоступный публичному memcpy_far.
; AF/HL разрушены, BC/DE сохранены. Промежуточного селектора в A/C нет.

;----------------------------------------------------------------------------
; MXOS
;
; Перевести код FAR-страницы в пару для аппаратного переключения.
;
; вход:
;   a = логическая страница: 0 для MAIN, положительный код для ДОЗУ; 80h..8Fh
;   используются внутренним ROM-профилем.
;
; выход:
;   l = младший байт адреса регистра маппера, h = значение для выбора физической
;   страницы. bc и de сохранены. af и прежний hl не сохраняются.
;----------------------------------------------------------------------------

bankCopyPair:
    or a
    jp m,bankCopyRomPair
    ld h,a
    ld l,0FCh
    ret z
    dec h
    inc l
    ret
bankCopyRomPair:
    and 00Fh
    ld h,a
    ld l,0FEh
    ret

; SP и длина остатка остаются в странице инициатора.
bankCopySP   = MEM_COPY_STATE
bankCopyRest = MEM_COPY_STATE+2

; BC/DE указывают на первые непереданные байты.
bankCopyReturn:
    ld a,(bankCopyRest)
    or a
    jp z,bankCopyRemoteDone
    ld (BANK_COPY_TINY_COUNT),a
    xor a
    ld (bankCopyRest),a
    ld hl,0
    add hl,de
    ld sp,hl
    ld hl,(bankCopySourcePair)
    ld a,h
    ld h,0FFh
    jp bankCopyTinySwitch
bankCopyRemoteDone:
    ld hl,(bankCopySP)
    ld sp,hl
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

bankCopyRange:
    ex de,hl
    ld hl,(bankCopyRequest+6)
    add hl,de
    jp c,bankCopyBadRange
    ld a,high MEM_DATA_LIMIT
    cp h
    ld a,BANK_ERROR_RANGE
    ret nz                     ; CF установлен, если старший байт конца выше границы.
    ld a,l
    or a
    ret z
bankCopyBadRange:
    ld a,BANK_ERROR_RANGE
    scf
    ret

bankCopyRequest: DS 8,0

; Ошибка стандартного PRINT_STRING: пакет не меняем, возвращаем код отказа.
bankBiosStringError:
    pop bc
    ld a,DOS_ERR_ARGUMENT
    ld (BANK_FRAME_STATUS),a
    scf
    jp MEM_BIOS_RESULT
