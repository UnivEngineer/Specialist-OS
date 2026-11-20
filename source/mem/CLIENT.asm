; Публичные входы и TEXT; включается в единую сборку LOCALBLOCK.
    ORG mx_call
    jp MEM_CALL_ENTRY
    jp bankCpmCall
    jp bankMxCallRegs
    jp bankQueueBind
    jp bankQueueBefore
    jp bankCopyFar
    jp MEM_LOCAL_INFO_ADDR
    jp MEM_EXTENSION_ENTRY
    jp memRouteCall

; B=группа, C=номер, DE=MX_REGS в исходной странице.
; AF/BC/DE/HL — результат исполнителя. SP и страница сохраняются.
; BIOS выполняется в main, файловые функции — в физическом банке ДОЗУ 0.
bankMxCall:
    ld a,b
    cp MXOS_GROUP_BIOS
    jp z,bankMxCheckPacket
    cp MXOS_GROUP_DOS
    jp nz,bankMxUnsupported
bankMxCheckPacket:
bankMxPacketReady:
    push de
    ld a,b
    cp MXOS_GROUP_DOS
    jp z,bankMxDos
    jp MEM_BIOS_ROUTE
bankMxDos:
    ld a,(bankPage)
    cp 1
    jp z,bankMxLocalDos
    ld a,1
    ld hl,BANK_DOS_GATE
bankMxInvoke:
    call bankCallPacket
    jp bankMxResult
bankMxLocalDos:
    call BANK_DOS_GATE+3
bankMxResult:
    push af
    push bc
    push de
    push hl
    ld a,(BANK_FRAME_STATUS)
    or a
    jp nz,bankMxReturned
    ld hl,8
    add hl,sp
    ld e,(hl)
    inc hl
    ld d,(hl)
    ld hl,0
    add hl,sp
    ld b,MX_REGS
bankMxSave:
    ld a,(hl)
    ld (de),a
    inc hl
    inc de
    dec b
    jp nz,bankMxSave
bankMxReturned:
    pop hl
    pop de
    pop bc
    pop af
    inc sp
    inc sp
    ret
bankCpmCall:
bankMxUnsupported:
    ld a,DOS_ERR_UNSUPPORTED
    scf
    ret

; CALL F806h / DW operation. Пакет расположен на стеке вызывающего банка.
; Ни один регистр не занят номером функции в пользовательском коде.
bankMxCallRegs:
    push af
    push bc
    push de
    push hl
    ld hl,8
    add hl,sp
    ld e,(hl)
    inc hl
    ld d,(hl)
    ex de,hl
    ld c,(hl)
    inc hl
    ld b,(hl)
    inc hl
    ex de,hl
    ld (hl),d
    dec hl
    ld (hl),e
    ld hl,0
    add hl,sp
    ex de,hl
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
memBiosQueue:
    ld hl,4
    add hl,de
    ld a,(hl)
    dec a
    cp 20
    jp nc,memBiosGeneric
    inc hl
    ld a,(hl)
    or a
    jp nz,memBiosGeneric
    ld a,7
    jp memBiosText
    ASSERT $ <= MEM_TEXT_ENTRY
    DS MEM_TEXT_ENTRY-$,0
    INCLUDE "text.inc"
    ASSERT $ <= MEM_SCOPE_ENTRY
    DS MEM_SCOPE_ENTRY-$,0
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
