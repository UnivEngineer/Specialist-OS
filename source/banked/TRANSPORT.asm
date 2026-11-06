    INCLUDE "abi.inc"
    ORG BANK_LANDING
; Тело копируется в каждую ИСПОЛНЯЕМУЮ страницу, включая main.
; До проверки kind и установки SP стек предыдущей страницы не трогаем.
bankLanding:
    ld a,(BANK_FRAME_KIND)
    cp 1
    jp z,bankResume
    ld a,(bankDepth)
    cp BANK_MAX_DEPTH
    jp nc,bankRejectStack
    ld hl,(bankStackFloor)
    ld d,h
    ld e,l
    ld hl,(bankServiceSP)
    ld a,h
    cp d
    jp c,bankRejectStack
    jp nz,bankEnter
    ld a,l
    cp e
    jp c,bankRejectStack
bankEnter:
    ld sp,hl
    ; Локальный кадр: предыдущий контекст, страница возврата, SP возврата.
    ld hl,(bankOwner)
    push hl
    ld a,(BANK_FRAME_PAGE)
    ld l,a
    ld h,0
    push hl
    ld hl,(BANK_FRAME_SP)
    push hl
    ld hl,(BANK_FRAME_OWNER)
    ld (bankOwner),hl
    ld hl,bankDepth
    inc (hl)
    ld hl,bankServiceReturn
    push hl
    ; Отмечаем занятую часть служебного стека ДО входа в программу:
    ; она может заменить SP своим стеком под экраном.
    ld hl,0
    add hl,sp
    ld (bankServiceSP),hl
    ld a,(BANK_FRAME_KIND)
    cp 2
    jp z,bankPacketEnter
    ld hl,(BANK_FRAME_REGS+4)
    ld b,h
    ld c,l
    ld hl,(BANK_FRAME_REGS+2)
    ex de,hl
    ld a,(BANK_FRAME_PAGE)
    ld hl,(BANK_FRAME_REGS)
    jp (hl)
bankPacketEnter:
    ld hl,(BANK_FRAME_OPERATION)
    ld b,h
    ld c,l
    ld de,BANK_FRAME_REGS
    ld a,(BANK_FRAME_PAGE)
    ld hl,(BANK_FRAME_ENTRY)
    jp (hl)

; Вызов: A=логическая страница назначения, HL=адрес процедуры в ней,
; DE=NEAR пакета исходной страницы, BC=операция. Целевая процедура получает
; A=логическая страница источника, BC/DE без изменений; HL=адрес входа.
; Выход: AF/BC/DE/HL целевой процедуры; исходные SP/страница/контекст сохранены.
; Прерывания запрещены. Прямой вход в ядро старого DOS здесь не допускается.
bankCall:
    ld (BANK_FRAME_TARGET),a
    push af
    ld a,(bankPageCount)
    ld (bankLimit+1),a
    pop af
bankLimit:
    cp 16+1
    jp c,bankCallValid
    jp z,bankCallValid
    ld a,BANK_ERROR_PAGE
    scf
    ret
bankCallValid:
    ; Сначала сохраняем регистры, затем предыдущую вершину служебного стека.
    push af
    push bc
    push de
    push hl
    ld hl,0
    add hl,sp
    ld (BANK_FRAME_SP),hl
    ld de,BANK_FRAME_REGS
    ld b,8
    call bankCopyBytes
    xor a
    ld (BANK_FRAME_KIND),a
bankCallSaved:
    ld hl,(bankServiceSP)
    ; Старый SP хранится отдельным словом ниже регистров; возвратный SP
    ; указывает на это слово, а восстановление пропускает его перед POP.
    push hl
    ld hl,0
    add hl,sp
    ld (BANK_FRAME_SP),hl
    ld a,(bankDepth)
    or a
    jp z,bankCallContext
    ld a,(bankPage)
    or a
    jp nz,bankCallServiceStack
    ; В main обычный стек программы расположен ниже экрана 9000h.
    ; Сохраняем его только в кадре возврата, не меняя вершину BIOS E400h.
    ; Истинное переполнение служебного стека (E3xx) по-прежнему отклоняется.
    ld a,h
    cp 090h
    jp c,bankCallContext
bankCallServiceStack:
    ld (bankServiceSP),hl
bankCallContext:
    ld a,(bankPage)
    ld (BANK_FRAME_PAGE),a
    ld hl,(bankOwner)
    ld (BANK_FRAME_OWNER),hl
    ld a,(BANK_FRAME_TARGET)
    jp bankSwitch

; A=назначение, HL=обработчик пакета, BC=операция, DE=локальный MX_REGS.
; Обработчик получает A=источник, BC=операция, DE=пакет прямо в общем окне.
; STATUS=1 до вызова; обработчик ставит 0 после успешной диспетчеризации.
; Результаты возвращаются в CPU. Клиент сам сохраняет их в локальный пакет.
bankCallPacket:
    ld (BANK_FRAME_TARGET),a
    push af
    ld a,1
    ld (BANK_FRAME_STATUS),a
    ld a,(bankPageCount)
    ld (bankPacketLimit+1),a
    pop af
bankPacketLimit:
    cp 16+1
    jp c,bankPacketValid
    jp z,bankPacketValid
    ld a,BANK_ERROR_PAGE
    scf
    ret
bankPacketValid:
    ld (BANK_FRAME_ENTRY),hl
    push af
    push bc
    push de
    push hl
    ld h,b
    ld l,c
    ld (BANK_FRAME_OPERATION),hl
    ex de,hl
    ld de,BANK_FRAME_REGS
    ld b,8
    call bankCopyBytes
    ld a,2
    ld (BANK_FRAME_KIND),a
    jp bankCallSaved

; Обратный регистровый вызов: bankCallPacket, BC=адрес обработчика
; в странице назначения, DE=MX_REGS. Указатели в пакете не преобразуются:
; вызывающая служба обязана передать скаляры либо NEAR целевой страницы.
; Возврат обычным RET; результат AF/BC/DE/HL, контекст сохраняет транспорт.
bankCallback:
    push bc
    xor a
    ld (BANK_FRAME_STATUS),a
    ld hl,(BANK_FRAME_REGS+6)
    push hl
    ld hl,(BANK_FRAME_REGS+4)
    ld b,h
    ld c,l
    ld hl,(BANK_FRAME_REGS+2)
    ex de,hl
    ld hl,(BANK_FRAME_REGS)
    pop af
    ret

; Сохранение результатов до чтения/изменения служебного кадра.
bankServiceReturn:
    push af
    push bc
    push de
    push hl
    ld hl,0
    add hl,sp
    ld de,BANK_FRAME_REGS
    ld b,8
    call bankCopyBytes
    pop hl
    pop de
    pop bc
    pop af
    pop hl
    ld (BANK_FRAME_SP),hl
    pop hl
    ld a,l
    ld (BANK_FRAME_TARGET),a
    pop hl
    ld (bankOwner),hl
    ld hl,0
    add hl,sp
    ld (bankServiceSP),hl
    ld hl,bankDepth
    dec (hl)
    ld a,1
    ld (BANK_FRAME_KIND),a
    ld a,(BANK_FRAME_TARGET)
    jp bankSwitch

bankResume:
    ld hl,(BANK_FRAME_SP)
    ld sp,hl
    pop hl
    ld (bankServiceSP),hl
    ld hl,0
    add hl,sp
    ex de,hl
    ld hl,BANK_FRAME_REGS
    ld b,8
    call bankCopyBytes
    pop hl
    pop de
    pop bc
    pop af
    ret

; Ошибка входа не использует стек отвергнувшей страницы.
bankRejectStack:
    ld a,BANK_ERROR_STACK
    ld (BANK_FRAME_REGS+7),a
    ld a,3
    ld (BANK_FRAME_REGS+6),a
    ld a,1
    ld (BANK_FRAME_KIND),a
    ld a,(BANK_FRAME_PAGE)
bankSwitch:
    or a
    jp z,BANK_SELECT_MAIN
    dec a
    jp BANK_SELECT_RAM

; HL=источник, DE=назначение, B=1..255. AF/B/DE/HL разрушены.
bankCopyBytes:
    ld a,(hl)
    ld (de),a
    inc hl
    inc de
    dec b
    jp nz,bankCopyBytes
    ret

bankPage:       DB 0
bankPageCount:  DB 16
bankOwner:      DB 0
bankHome:       DB 0
bankDepth:      DB 0
bankServiceSP:  DW BANK_STACK_TOP
; Нижняя граница с запасом кадра задаётся отдельно в каждой странице.
; У BIOS и DOS разные стеки; F800h не является универсальной границей.
bankStackFloor: DW BANK_STACK_LOW+64
bankTransportEnd:
    ASSERT bankTransportEnd <= 0FF00h
    INCLUDE "copy.inc"
    ASSERT $ <= 0FF00h
    DS 0FF00h-$,0
    INCLUDE "copytail.inc"
    ASSERT $ <= BANK_WINDOW
