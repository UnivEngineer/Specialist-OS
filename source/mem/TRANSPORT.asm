    INCLUDE "../include/mxos.inc"
    INCLUDE "abi.inc"
    INCLUDE "../../bin/banked/main-io-import.inc"
    ORG BANK_LANDING
; Тело копируется в каждый исполняемый банк и в основную память.
; Раскладки локальных стековых кадров: BANK_CALLER_* / BANK_CALLEE_* в abi.inc.
; До проверки kind и установки SP стек предыдущей страницы не трогаем.
    DS 7,0
bankLanding:
    dec a
    jp z,bankResume
    ld hl,(bankServiceSP)
bankEnter:
    ld sp,hl
    ; Локальный кадр исполнителя: 8 байт, раскладка BANK_CALLEE_* в abi.inc.
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
    call bankPublishSP
    ; BC и DE уже содержат операцию и указатель в форме исполнителя.
    ld a,(BANK_FRAME_PAGE)
    ld hl,(BANK_FRAME_ENTRY)
    jp (hl)

; Вызов: A=логическая страница назначения, HL=адрес процедуры в ней,
; DE=NEAR пакета исходной страницы, BC=операция. Целевая процедура получает
; A=логическая страница источника, BC/DE без изменений; HL=адрес входа.
; Выход: AF/BC/DE/HL целевой процедуры; исходные SP/страница/контекст сохранены.
; Прерывания запрещены. Прямой вход в ядро старого DOS здесь не допускается.

;----------------------------------------------------------------------------
; MXOS
;
; Вызвать обработчик в другой странице и вернуться в контекст инициатора.
;
; вход:
;   a = логическая страница назначения, hl = адрес исполняемой процедуры в ней, de =
;   NEAR пакета в странице источника, bc = операция. Прерывания выключены.
;
; выход:
;   Целевая процедура получает a = страница источника, прежние bc и de, hl = адрес
;   входа. На возврате af, bc, de и hl передают результат обработчика, страница и SP
;   инициатора восстановлены.
;----------------------------------------------------------------------------

bankCall:
    ld (BANK_FRAME_TARGET),a
    ; BC и DE доходят до процедуры в CPU. HL хранит адрес входа.
    ld (BANK_FRAME_ENTRY),hl
    xor a
    ld (BANK_FRAME_KIND),a
bankCallSaved:
    ld hl,(bankServiceSP)
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

;----------------------------------------------------------------------------
; MXOS
;
; Передать пакет регистров через общее банковое окно COMMAND.
;
; вход:
;   a = страница назначения, hl = обработчик, bc = операция, de = адрес MX_REGS в
;   локальной странице.
;
; выход:
;   Обработчик получает a = страница источника, de = адрес пакета в общем окне, bc =
;   операция. Итог возвращается в регистрах процессора. Для записи результата в
;   исходный пакет вызывающая сторона сохраняет регистры сама.
;----------------------------------------------------------------------------

bankCallPacket:
    ld (BANK_FRAME_TARGET),a
    ld a,1
    ld (BANK_FRAME_STATUS),a
bankPacketValid:
    ld (BANK_FRAME_ENTRY),hl
    ; От первого SPHL до восстановления SP нет CALL, PUSH или прерываний.
    ld hl,0
    add hl,sp
    ld (bankPacketSP+1),hl
    ex de,hl
    ld sp,hl
    DUP 4, wordIndex
        pop hl
        ld (BANK_FRAME_REGS+2*wordIndex),hl
    EDUP
bankPacketSP:
    ld sp,0
    ld de,BANK_FRAME_REGS
    xor a
    ld (BANK_FRAME_KIND),a
    jp bankCallSaved

;----------------------------------------------------------------------------
; MXOS
;
; Передать управление обработчику обратного вызова с регистрами из общего окна.
;
; вход:
;   Вызов через bankCallPacket, bc = адрес обработчика в текущей целевой странице.
;   BANK_FRAME_REGS содержит слова HL, DE, BC и AF. Указатели должны относиться к
;   странице обработчика.
;
; выход:
;   BANK_FRAME_STATUS обнулён. Обработчик получает регистры из пакета и возвращается
;   через RET в транспорт. Его af, bc, de и hl составляют результат.
;----------------------------------------------------------------------------

bankModuleCallback:
bankCallback:
    ld hl,bankModuleReturned
    push hl
    push bc
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
    ; BC и DE проходят обратное переключение в регистрах.
    ld (BANK_FRAME_REGS),hl
    push af
    pop hl
    ld (BANK_FRAME_REGS+6),hl
    pop hl
    ld (BANK_FRAME_SP),hl
    pop hl
    ld a,l
    pop hl
    ld (bankOwner),hl
    call bankPublishSP
    ld hl,bankDepth
    dec (hl)
    ld hl,BANK_FRAME_KIND
    ld (hl),1
    jp bankSwitch

bankResume:
    ld hl,(BANK_FRAME_SP)
    ld sp,hl
    pop hl
    ld (bankServiceSP),hl
    ld hl,(BANK_FRAME_REGS+6)
    push hl
    pop af
    ld hl,(BANK_FRAME_REGS)
    ret

bankSwitch:
    or a
    ld hl,BANK_IO_MAIN
    jp z,BANK_SELECT_MAIN
    dec a
    inc hl
    jp BANK_SELECT_RAM

; HL=источник, DE=назначение, B=1..255. AF/B/DE/HL разрушены.
bankCopyBytes = MEM_COPY_BYTES_ENTRY
bankCopyQuads = MEM_COPY_QUADS_ENTRY
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
; Модуль вернулся обычным RET. Вложенный отказ BIOS/DOS не должен
; оставлять статус диспетчеризации внешнего moduleCall незавершённым.
bankModuleReturned:
    push af
    xor a
    ld (BANK_FRAME_STATUS),a
    pop af
    ret
; SP исполнителя до CALL. Указатель переживает переход программы на свой стек.
bankPublishSP:
    ld hl,2
    add hl,sp
    ld (bankServiceSP),hl
    ret
; Локальная диспетчеризация B=5; не переключает страницу/профиль.
bankLocalChecked:
    ld a,(bankPage)
    or a
    jp nz,MEM_IO_CHECKED
    push hl
    ld hl,(MEM_LOCAL_CHECKED)
    ex (sp),hl
    ret
bankLocalBlockBad:
    ld a,DOS_ERR_ARGUMENT
    scf
    ret
    INCLUDE "copy.inc"
;----------------------------------------------------------------------------
; MXOS
; Войти в листовую службу BIOS/DOS с регистровым ABI.
; вход BIOS: AF/DE/HL — аргументы, на стеке продолжение за DW.
; В DOS-входе HL указывает на DW, в стеке лежат AF и исходный HL.
; выход: AF/HL службы, исходные BC/DE и SP.
;----------------------------------------------------------------------------
bankMoveToMain:
    ld (BANK_IO_MAIN),a
    jp bankCopyMoveRemote
    ASSERT $ <= MEM_DATA_START-3
    IF $ < MEM_DATA_START-3
    DS MEM_DATA_START-3-$,0
    ENDIF
bankMoveToRam:
    ld (BANK_IO_RAM),a
    ASSERT $ == MEM_DATA_START
    ASSERT $ <= MEM_DATA_START
    IF $ < MEM_DATA_START
    DS MEM_DATA_START-$,0
    ENDIF
    INCLUDE "remote-move.inc"
    INCLUDE "local-move.inc"
coldEnd:
    INCLUDE "copy-resident.inc"
    ASSERT $ <= BANK_COMMAND_LANDING
    IF $ < BANK_COMMAND_LANDING
    DS BANK_COMMAND_LANDING-$,0
    ENDIF
    INCLUDE "scalar-command.inc"
    ASSERT $ <= 0FF00h
    IF $ < 0FF00h
    DS 0FF00h-$,0
    ENDIF
    INCLUDE "copytail.inc"
hotEnd:
    ASSERT $ <= MEM_EXEC_READY
    IF $ < MEM_EXEC_READY
    DS MEM_EXEC_READY-$,0
    ENDIF
    DB 0
bankDataTailEnd:
    ASSERT $ == BANK_WINDOW

    ORG 0F475h ; fileorg-ok
bankFastPrepare:
    push bc
    push af
    push hl
    ld hl,4
    add hl,sp
    ld (bankFastRestore+1),hl
    ld a,(bankPage)
    dec a
    ld b,a
    pop hl
    pop af
bankFastBiosSP:
    ld sp,BIOS_BANK_GATE_ADDR
    ld (BANK_IO_MAIN),a
bankFastLanding:
    ret                         ; Первое слово — вектор BIOS, второе — bankFastReturn.
bankFastDos:
    inc hl
    inc hl
    pop af
    ex (sp),hl
    ld a,(bankPage)
    dec a
    jp z,DOS_FAST_ERROR_ENTRY   ; В DOS вызов уже на своём стеке.
    push bc
    ld b,a
    push hl
    ld hl,2
    add hl,sp
    ld (bankFastRestore+1),hl
    pop hl
    ld sp,bankFastReturnPC
    xor a
    ld (BANK_IO_RAM),a
    jp DOS_FAST_ERROR_ENTRY
bankFastReturn:
    ld c,a
    ld a,b
    ld (BANK_IO_RAM),a
    ld a,c
bankFastRestore:
    ld sp,0
    pop bc
    ret
bankMxRegsDos:
    ld a,(bankPage)
    dec a                   ; DOS_BANK_PAGE = 1.
    jp z,BANK_DOS_GATE+3
    ld a,DOS_BANK_PAGE
    ld hl,BANK_DOS_GATE
    jp bankCallPacket
; HL = адрес локального callback сброса очереди, HL=0 отключает его.
; Обработчик должен вернуться через RET; вход сброса сохраняет регистры
; и флаги вызывающего и временно закрывает шлюз на время callback.
; Привязка меняет AF, сохраняет BC, DE, HL. Флаги статуса не возвращаются.
bankQueueBind:
    ld (MEM_QUEUE_HOOK),hl
    ld a,h
    or l
    ld a,0C9h
    jp z,bankQueueBound
    ld a,0C3h
bankQueueBound:
    ld (bankQueueBefore),a
    ret
    ; MEMTOP после выбора MAIN: CF=1 читает, CF=0 записывает, знак и A задают возврат.
    ASSERT $ <= MEM_TOP_MAPPED
    IF $ < MEM_TOP_MAPPED
    DS MEM_TOP_MAPPED-$,0
    ENDIF
    jp c,bankGetTopMapped
    ld (MAIN_MEM_TOP_TARGET),hl
    jp MEM_WORD_RETURN
bankGetTopMapped:
    ld hl,(MAIN_MEM_TOP_TARGET)
    jp MEM_WORD_RETURN
    INCLUDE "queue-hook.inc"
bankFastReturnPC:
    DW bankFastReturn
movedEnd:
