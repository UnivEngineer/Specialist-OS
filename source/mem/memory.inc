; Общие операции памяти; одинаковое тело в main и исполняемых банках.
; A=операция; MOVE: HL=MEM_COPY_REQUEST. GET/PUT: B=банк, HL=адрес,
; DE=значение (для байта E). GET возвращает DE; BC/HL сохранены.
; PRINT: HL=локальная ASCIIZ, возвращает HL на нуле, BC/DE сохранены.
    jp memMemory
    jp memCopySelect
    jp memBiosRoute
    jp bankMxResult
    jp memMovePrepared
memMemory:
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
memProfile:
    push bc
    push de
    push hl
    push af
    ld a,(BANK_PROFILE)
    or a
    jp z,memProfileSave
    cp BANK_PROFILE_COUNT
    jp c,memProfileReady
memProfileSave:
    ld hl,BANK_WINDOW
    ld de,bankCopySavedWindow
    ld b,32
    call bankCopyBytes
memProfileReady:
    pop af
    ld (BANK_PROFILE),a
    pop hl
    pop de
    pop bc
    ret

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
    ld a,b
    call bankCopyPageCheck
    jp c,memScalarError
    ld a,h
    cp 0FFh
    jp nc,memScalarRange
    ld a,(memOperation)
    and 1
    jp nz,memScalarValid
    inc hl
    ld a,h
    cp 0FFh
    dec hl
    jp nc,memScalarRange
memScalarValid:
    ; GET/PUT8/16 разделяют COPY с GET/PUT32 и memcpy.
    ld a,BANK_PROFILE_COPY
    call memProfile
    push de
    ld hl,memScalarTemplate
    ld de,BANK_WINDOW
    ld b,memScalarTemplateEnd-memScalarTemplate
    call bankCopyBytes
    pop de
memScalarInstalled:
    pop hl
    pop bc
    push bc
    push hl
    ld (BANK_WINDOW+4),hl
    ld a,b
    call bankCopyPair
    ld a,l
    ld (BANK_WINDOW+1),a
    ld a,h
    ld (memSelect),a
    ld a,(bankPage)
    call bankCopyPair
    ld a,l
    ld (BANK_WINDOW+12),a
    ld a,h
    ld (BANK_WINDOW+10),a
    ld a,(memOperation)
    cp MEM_OP_PUT8
    ld a,05Eh ; LD E,(HL)
    ld c,056h ; LD D,(HL)
    jp c,memScalarRead
    ld a,073h ; LD (HL),E
    ld c,072h ; LD (HL),D
memScalarRead:
    ld (BANK_WINDOW+6),a
    ld a,(memOperation)
    and 1
    jp z,memScalarWord
    ld c,0 ; NOP: не обращаться ко второму байту
memScalarWord:
    ld a,c
    ld (BANK_WINDOW+8),a
    ld a,(memSelect)
    jp BANK_WINDOW
memScalarReturn:
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
memScalarTemplate:
    ld (0FFFDh),a
    ld hl,0
    ld e,(hl)
    inc hl
    ld d,(hl)
    ld a,0
    ld (0FFFCh),a
    jp memScalarReturn
memScalarTemplateEnd:
    ASSERT memScalarTemplateEnd-memScalarTemplate <= 31

; TEXT20: общий путь строк и скаляров при уже активном TEXT.
memPrint:
    xor a
    ld (memOperation),a
memPrintTyped:
    push af
    ld a,(bankPage)
    or a
    jp z,memPrintNative
    push bc
    push de
    ld (memTextNext),hl
    ld a,(memOperation)
    cp 2
    ld hl,0FFFFh
    jp c,memPrintCount
    ld h,b
    ld l,c
memPrintCount:
    ld (memTextRemaining),hl
memPrintLoop:
    ld hl,(memTextRemaining)
    ld a,h
    or l
    jp z,memPrintDone
    ld b,20
    ld a,h
    or a
    jp nz,memPrintSize
    ld a,l
    cp 20
    jp nc,memPrintSize
    ld b,l
memPrintSize:
    ld a,BANK_PROFILE_TEXT
    call memProfile
    ld a,077h
    ld (BANK_WINDOW),a
    ld a,0C3h
    ld (BANK_WINDOW+1),a
    ld hl,memPrintMain
    ld (BANK_WINDOW+2),hl
    ld hl,(memTextNext)
    ld de,BANK_PACKET
memPrintFill:
    ld a,h
    cp 0F8h
    jp nc,memPrintFilled
    ld a,(hl)
memPrintStore:
    ld (de),a
    inc de
    inc hl
    dec b
    jp nz,memPrintFill
memPrintFilled:
    ld (memTextNext),hl
    xor a
    ld (de),a
    ld a,e
    sub BANK_PACKET & 0FFh
    or a
    jp z,bankTextRangeError
    ld c,a
    ld (memBack),a
    ld hl,0
    add hl,sp
    ld (memSavedSP),hl
    ld a,(memOperation)
    ld d,a
    ld a,(bankPage)
    ld b,a
    xor a
    ld hl,BANK_IO_MAIN
    jp BANK_WINDOW
memPrintMain:
    ld hl,(bankServiceSP)
    ld sp,hl
    push bc
    ld a,d
    ld hl,BANK_PACKET
    call bios_textDispatch
    ld de,-BANK_PACKET
    add hl,de
    ld a,l
    pop bc
    ld c,a
    ld hl,memPrintReturn
    ld (BANK_WINDOW+2),hl
    ld a,b
    dec a
    ld hl,BANK_IO_RAM
    jp BANK_WINDOW
memPrintReturn:
    ld hl,(memSavedSP)
    ld sp,hl
    ld a,(memBack)
    sub c
    jp nz,memPrintPartial
    ld a,(memOperation)
    cp 2
    jp c,memPrintLoop
    ld hl,(memTextRemaining)
    ld b,0
    ld a,l
    sub c
    ld l,a
    ld a,h
    sbc a,b
    ld h,a
    ld (memTextRemaining),hl
    jp memPrintLoop
memPrintPartial:
    ld hl,(memTextNext)
memPrintBack:
    dec hl
    dec a
    jp nz,memPrintBack
    jp memPrintHaveHL
memPrintDone:
    ld hl,(memTextNext)
memPrintHaveHL:
    pop de
    pop bc
    pop af
    or a
    ret
memPrintNative:
    pop af
    jp bios_printStringZ
memTextRemaining = memScalarTemplate+4
memSavedSP: DW 0
memTextNext: DW 0
memOperation: DB 0
memSelect: DB 0
memBack: DB 0
memTextScalar:
    push hl
    push bc
    ld hl,memSelect
    ld bc,1
    call memPrintTyped
    pop bc
    pop hl
    ret
memExtensionEnd:
