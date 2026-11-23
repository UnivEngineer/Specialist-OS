; Публичное FAR-копирование адресов; не malloc и не проверка владельца.
; HL -> {srcPage,srcOffset:word,dstPage,dstOffset:word,count:word}.
; Пакет находится в текущей странице. Диапазоны заканчиваются до FF00h;
; пересечение диапазонов одной страницы запрещено контрактом memcpy.
; AF=статус, CF=ошибка; BC/DE/HL, банк и SP сохранены; профиль окна остаётся COPY.
; Нереентерабельная атомарная петля: без callback и с запрещёнными IRQ.

;----------------------------------------------------------------------------
; MXOS
;
; Скопировать байты между страницами по запросу FAR.
;
; вход:
;   hl = локальный адрес восьми байт: страница источника, его смещение, страница
;   назначения, её смещение, длина. Смещения и длина — слова. Диапазоны ограничены
;   FF00h, перекрытие внутри одной страницы запрещено.
;
; выход:
;   cf = 0 при успехе, при отказе cf = 1 и a содержит ошибку MEM. Для нулевой длины
;   копирование пропускается. bc, de, hl, исходная страница и SP сохраняются.
;----------------------------------------------------------------------------

bankCopyFar:
    call bankQueueBefore
    xor a
    ld (bankCopyMove),a
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
    ld a,(bankCopyMove)
    or a
    jp nz,MEM_MOVE_SELECT_ADDR
    jp MEM_COPY_SELECT_ADDR
bankCopyPrepared:
    jp MEM_COPY_PREPARE_ENTRY

; Секторный внутренний вход: тот же пакет, ровно 256 байт.
; Драйвер уже проверил страницы/буфер. Сохраняет регистры, SP и окно,
; поэтому пригоден и для вложенных вызовов из DOS, и из main.
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

 ; Параметры инициатора не находятся в аппаратном окне.
bankCopySP    = MEM_COPY_STATE
bankCopyPairs = MEM_COPY_STATE+2
bankCopyRest  = MEM_COPY_STATE+8

; Полные 32-байтные порции закончены. BC/DE — первые непереданные адреса.
bankCopyReturn:
    ld hl,(bankCopySP)
    ld sp,hl
    ld a,(bankCopyRest)
    or a
    jp z,bankCopyBulkDone
    push bc
    push de
    ld hl,bankCopyTinyTemplate
    ld de,BANK_WINDOW
    ld b,8
    call bankCopyQuads
    ld hl,(bankCopyPairs)
    ld a,h
    ld (BANK_WINDOW+4),a
    ld h,0FFh
    ld (BANK_WINDOW+1),hl
    ld hl,(bankCopyPairs+2)
    ld a,h
    ld (BANK_WINDOW+11),a
    ld h,0FFh
    ld (BANK_WINDOW+8),hl
    ld hl,(bankCopyPairs+4)
    ld a,h
    ld (BANK_WINDOW+25),a
    ld h,0FFh
    ld (BANK_WINDOW+22),hl
    ld a,(bankCopyRest)
    ld (BANK_WINDOW+29),a
    pop de
    pop bc
    jp BANK_WINDOW
bankCopyBulkDone:
    ld a,BANK_PROFILE_COPY
    ld (BANK_PROFILE),a
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
    ld a,0FFh
    cp h
    ret nz                     ; H<FFh, поэтому CF=0.
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

; Следующая команда или возврат из обработчика восстанавливает его окно.
; AF/BC/DE/HL сохранены; стек уже должен принадлежать текущему банку.
bankCommandWindow:
    push af
    ld a,(BANK_PROFILE)
    or a
    jp z,bankWindowReady
    xor a
    call MEM_SET_PROFILE_ENTRY
bankWindowReady:
    xor a
    ld (BANK_BUFFER_KIND),a
    pop af
    ret

bankCopyTinyTemplate = MEM_COPY_TINY_TEMPLATE
bankCopyRequest: DS 8,0
bankCopySavedWindow = MEM_COMMAND_SHADOW

; Ошибка стандартного PRINT_STRING: пакет не меняем, возвращаем код отказа.
bankBiosStringError:
    call bankCommandWindow
    pop bc
    ld a,DOS_ERR_ARGUMENT
    ld (BANK_FRAME_STATUS),a
    scf
    jp MEM_BIOS_RESULT

bankCopyMove: DB 0
