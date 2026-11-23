    INCLUDE "../include/mxos.inc"
    INCLUDE "abi.inc"
    ORG BANK_LANDING
; Тело копируется в каждый исполняемый банк и в основную память.
; Раскладки локальных стековых кадров: BANK_CALLER_* / BANK_CALLEE_* в abi.inc.
; До проверки kind и установки SP стек предыдущей страницы не трогаем.
bankLanding:
    ld a,(BANK_BUFFER_KIND)
    or a
    jp z,bankFrameLanding
    dec a
    jp z,MEM_BUFFER_MAIN_ENTRY
    jp MEM_BUFFER_RETURN_ENTRY
bankFrameLanding:
    ld a,(BANK_FRAME_KIND)
    cp 1
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
    call bankCommandWindow
    ld (BANK_FRAME_TARGET),a
bankCallValid:
    ; Кадр вызывающей стороны: 12 байт с адресом от CALL; BANK_CALLER_* в abi.inc.
    push af
    push bc
    push de
    push hl
    ld hl,0
    add hl,sp
    ld (BANK_FRAME_SP),hl
    ld de,BANK_FRAME_REGS
    ld b,2
    call bankCopyQuads
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
    ; Горячий COMMAND не вызывает подпрограмму установки профиля.
    push af
    ld a,(BANK_PROFILE)
    or a
    call nz,bankCommandWindow
    xor a
    ld (BANK_BUFFER_KIND),a
    pop af
    ld (BANK_FRAME_TARGET),a
    push af
    ld a,1
    ld (BANK_FRAME_STATUS),a
    pop af
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
    ld b,2
    call bankCopyQuads
    ld a,2
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
    ; Горячий COMMAND не вызывает подпрограмму установки профиля.
    push af
    ld a,(BANK_PROFILE)
    or a
    call nz,bankCommandWindow
    xor a
    ld (BANK_BUFFER_KIND),a
    pop af
    ; После сохранения результата CPU-регистры уже не нужны: пишем слова
    ; прямо в окно, без восьмибайтной копии со стека и обратных POP.
    ld (BANK_FRAME_REGS),hl
    ex de,hl
    ld (BANK_FRAME_REGS+2),hl
    ld h,b
    ld l,c
    ld (BANK_FRAME_REGS+4),hl
    push af
    pop hl
    ld (BANK_FRAME_REGS+6),hl
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
    ld b,2
    call bankCopyQuads
    pop hl
    pop de
    pop bc
    pop af
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
bankModuleCallback:
    call bankCallback
bankModuleReturned:
    push af
    xor a
    ld (BANK_FRAME_STATUS),a
    pop af
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
; Ошибка строки: стек содержит сохранённые DE/BC/AF, затем возврат.
bankTextRangeError:
    pop de
    pop bc
    pop af
    ld a,DOS_ERR_ARGUMENT
    scf
    ret
    INCLUDE "copy.inc"
    INCLUDE "queue-hook.inc"
    ASSERT $ <= BANK_COMMAND_LANDING
    DS BANK_COMMAND_LANDING-$,0
    INCLUDE "scalar-command.inc"
    ASSERT $ <= 0FF00h
    DS 0FF00h-$,0
    INCLUDE "copytail.inc"
    ASSERT $ <= BANK_WINDOW
