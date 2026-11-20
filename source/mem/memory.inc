; Общие операции памяти; одинаковое тело в main и исполняемых банках.
; A=операция; MOVE: HL=MEM_COPY_REQUEST. GET/PUT: B=банк, HL=адрес,
; DE=значение (для байта E). GET возвращает DE; BC/HL сохранены.
; PRINT: HL=локальная ASCIIZ, возвращает HL на нуле, BC/DE сохранены.
    jp memMemory
    jp memCopySelect
    jp memBiosRoute
    jp bankMxResult
    jp memMovePrepared
    jp memCommandScalarReturn
    jp memProfile
    jp memPrintMain
    jp memPrintReturn
memProfile:
    push af
    push bc
    push de
    push hl
    ld c,a
    ld a,(BANK_PROFILE)
    or a
    jp nz,memProfileFromOther
    ld a,c
    or a
    jp z,memProfileReady
    ld hl,BANK_WINDOW
    ld de,bankCopySavedWindow
    jp memProfileCopy
memProfileFromOther:
    ld a,c
    or a
    jp nz,memProfileReady
    ld hl,bankCopySavedWindow
    ld de,BANK_WINDOW
memProfileCopy:
    ld b,8
    call bankCopyQuads
memProfileReady:
    ld a,c
    ld (BANK_PROFILE),a
    pop hl
    pop de
    pop bc
    pop af
    ret
memMemory:
    call bankQueueBefore
    or a
    jp z,memMove
    cp MEM_OP_PRINT
    jp z,memPrint
    cp MEM_OP_PRINT+1
    jp nc,memBad
    jp memScalar
memBad:
    ld a,MEM_ERROR_RANGE
    scf
    ret

; Сохраняется канонический COMMAND; COPY/TEXT/LOCAL разделяют снимок.
; A=новый профиль; BC/DE/HL сохранены, AF разрушены.
; Вход после проверки пакета memcpy. Регистры уже сохранены bankCopyFar.
memCopySelect:
    ld a,(bankCopyRequest)
    ld b,a
    ld a,(bankCopyRequest+3)
    cp b
    jp nz,bankCopyPrepared
    ld a,(bankPage)
    cp b
    jp nz,bankCopyPrepared
    ld hl,(MEM_NEAR_COPY)
    ld a,h
    or l
    jp z,bankCopyPrepared
memNativeCopy:
    ; Штатный локальный стековый движок: HL=начало, DE=конец, BC=приёмник.
    ; Указатель задаётся установщиком в контексте текущего банка.
    ld hl,(bankCopyRequest+6)
    ex de,hl
    ld hl,(bankCopyRequest+1)
    add hl,de
    ex de,hl
    ld hl,(bankCopyRequest+4)
    ld b,h
    ld c,l
    ld hl,(bankCopyRequest+1)
    call memNearInvoke
    jp bankCopyOK
memNearInvoke:
    push hl
    ld hl,(MEM_NEAR_COPY)
    ex (sp),hl
    ret

; Перекрывающаяся копия одного банка: окно само исполняет цикл и возврат.
memLocalCopy:
    ld a,BANK_PROFILE_LOCAL
    call memProfile
    ld hl,memLocalTemplate
    ld de,BANK_WINDOW
    ld b,memLocalTemplateEnd-memLocalTemplate
    call bankCopyBytes
    ld a,(bankCopyRequest)
    call bankCopyPair
    ld a,l
    ld (BANK_WINDOW+1),a
    ld a,h
    ld (memSelect),a
    ld a,(bankPage)
    call bankCopyPair
    ld a,l
    ld (BANK_WINDOW+16),a
    ld a,h
    ld (BANK_WINDOW+14),a
    ld hl,(bankCopyRequest+6)
    ld b,h
    ld c,l
    ld hl,(bankCopyRequest+4)
    ex de,hl
    ld hl,(bankCopyRequest+1)
    ld a,(memBack)
    or a
    jp z,memLocalForward
    add hl,bc
    dec hl
    ex de,hl
    add hl,bc
    dec hl
    ex de,hl
    ld a,02Bh ; DEC HL
    ld (BANK_WINDOW+5),a
    ld a,01Bh ; DEC DE
    ld (BANK_WINDOW+6),a
memLocalForward:
    ld a,(memSelect)
    jp BANK_WINDOW
memLocalReturn = bankCopyOK
memLocalTemplate:
    ld (0FFFDh),a
    ld a,(hl)
    ld (de),a
    inc hl
    inc de
    dec bc
    ld a,b
    or c
    jp nz,BANK_WINDOW+3
    ld a,0
    ld (0FFFCh),a
    jp memLocalReturn
memLocalTemplateEnd:
    ASSERT memLocalTemplateEnd-memLocalTemplate <= 31

memMove:
    ld a,1
    ld (bankCopyMove),a
    jp bankCopyEnter
memMovePrepared:
    ld a,(bankCopyRequest)
    ld b,a
    ld a,(bankCopyRequest+3)
    cp b
    jp nz,bankCopyPrepared
    ld a,(bankPage)
    cp b
    jp nz,memMoveRemote
    ld hl,(MEM_NEAR_COPY)
    ld a,h
    or l
    jp nz,memNativeCopy
memMoveRemote:
    ; Обратное направление безопасно и без фактического пересечения.
    ld hl,(bankCopyRequest+1)
    ex de,hl
    ld hl,(bankCopyRequest+4)
    ld a,l
    sub e
    ld a,h
    sbc a,d
memMoveDirection:
    ld a,0
    jp c,memMoveReady
    inc a
memMoveReady:
    ld (memBack),a
    jp memLocalCopy
memMoveError = bankCopyError

memScalar:
    push bc
    push hl
    ld (memOperation),a
memScalarValid:
    ld (memScalarAddress),hl
    ld a,b
    ld (memScalarBank),a
    ex de,hl
    ld (memScalarValue),hl
    ex de,hl
    ld a,(BANK_PROFILE)
    or a
    call nz,bankCommandWindow  ; COPY/TEXT/LOCAL: вернуть обычный COMMAND.
    xor a
    ld (BANK_BUFFER_KIND),a
    ld a,(BANK_SCALAR_KIND)
    push af
    ld a,(bankPage)
    call bankCopyPair
    ld (BANK_SCALAR_RETURN_PAIR),hl
    ld a,(memOperation)
    dec a
    ld e,a
    ld d,0
    ld hl,memScalarKinds
    add hl,de
    ld a,(hl)
    ld (BANK_SCALAR_KIND),a
    ld hl,(memScalarValue)
    ld b,h
    ld c,l
    ld hl,(memScalarAddress)
    ex de,hl
    ld a,(memScalarBank)
    call bankCopyPair
    ld a,h
    ld h,0FFh
    jp BANK_WINDOW
memCommandScalarReturn:
    pop af
    ld (BANK_SCALAR_KIND),a
    ld d,b
    ld e,c
    ld a,(memOperation)
    cp MEM_OP_GET8
    jp nz,memScalarDone
    ld d,0
memScalarDone:
    xor a
memScalarError:
    pop hl
    pop bc
    ret
memScalarRange:
    pop hl
    pop bc
    jp memBad
memScalarKinds: DB 5,3,6,4
memScalarAddress: DW 0
memScalarValue: DW 0
memScalarBank: DB 0

memRouteInvoke:
    call bankCallPacket
    ex (sp),hl
    push af
    ld a,h
    ld (bankHome),a
    pop af
    ex (sp),hl
    inc sp
    inc sp
    ret
memRouteBad:
    ld a,MEM_ERROR_HANDLE
    scf
    ret
memRoute = MEM_ROUTE_STATE
memExtensionEnd:
