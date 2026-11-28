; Таблица JMP полного профиля MEM; включается в сборку LOCALBLOCK.
; От mx_callPacket: +00h пакетный вызов, +03h зарезервированный отказ CP/M,
; +06h вызов с DW операции, +09h привязка очереди, +0Ch сброс очереди,
; +0Fh FAR-копирование, +12h запрос MEM, +15h локальные операции памяти,
; +18h вызов DLL по текущему маршруту MODULE_SCOPE. Контракты в api.md.
    ASSERT $ == mx_callPacket
    jp MEM_CALL_PACKET_ENTRY
    jp bankCpmCall
    jp bankMxCallRegs
    jp bankQueueBind
    jp bankQueueBefore
    jp bankCopyFar
    jp MEM_LOCAL_INFO_ADDR
    jp MEM_EXTENSION_ENTRY
    jp memRouteCall
    ASSERT $ == mx_printChar
    jp memFixedPrint
    jp memFixedGetCursor
    jp memFixedSetCursor
    jp memFixedRaw
    jp memFixedHexByte
    jp memFixedHexWord
    ASSERT $ == mx_getMemTop
    jp memFixedGetTop
    jp memFixedSetTop

;----------------------------------------------------------------------------
; MXOS
;
; Выполнить системный вызов mx_callRegs с номером операции в потоке кода.
;
; вход:
;   Сразу после CALL mx_callRegs следует DW операции (группа в старшем байте).
;   Листовые службы принимают аргументы в CPU, общий маршрут использует MX_REGS.
;
; выход:
;   AF, BC, DE и HL содержат результат, включая флаги операции. Возврат
;   происходит за DW. SP и банк вызывающего восстанавливаются.
;----------------------------------------------------------------------------
bankMxCallRegs:
    call bankQueueBefore
    push af
    ld a,(bankPage)
    or a
    jp nz,bankFastDecode
bankMxCallRegsSaved:
    push bc
    push de
    push hl
    ld hl,8
    add hl,sp
    ld sp,hl
    pop hl                  ; Адрес DW непосредственно из кадра CALL.
    ld c,(hl)
    inc hl
    ld b,(hl)
    inc hl
    push hl                 ; Адрес продолжения занимает прежнее место.
    ld hl,-8
    add hl,sp
    ld sp,hl
    ex de,hl
bankMxRegsRouteCall:
    call bankMxRegsRoute
    inc sp
    inc sp
    inc sp
    inc sp
    inc sp
    inc sp
    inc sp
    inc sp
    ret
    ASSERT $ == MEM_TEXT_ENTRY
    INCLUDE "text.inc"
    ASSERT $ <= MEM_SCOPE_ENTRY
    IF $ < MEM_SCOPE_ENTRY
    DS MEM_SCOPE_ENTRY-$,0
    ENDIF
; BC=controller callback. Keep the enclosing route on this service stack.
memScopeEnter:
    ld hl,(memRoute)
    push hl
    ld hl,(memRoute+2)
    push hl
    ld hl,(memRoute+4)
    push hl
    push bc
    ld hl,BANK_FRAME_REGS
    ld de,memRoute
    ld b,6
    call bankCopyBytes
    pop hl
    call memScopeInvoke
    call bankQueueBefore
    pop hl
    ld (memRoute+4),hl
    pop hl
    ld (memRoute+2),hl
    pop hl
    ld (memRoute),hl
    ret
memScopeInvoke:
    jp (hl)
bankClientEnd:
    ASSERT $ <= LOCAL_DRIVER_ADDR
