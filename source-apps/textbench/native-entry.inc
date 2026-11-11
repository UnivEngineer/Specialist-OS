; Штатный BANK COM. Аргумент — четыре hex-цифры адреса согласованного
; блока main: MX_REGS, резерв:word, corpus:word, FAR-код:byte, резерв.
; Загрузка/возврат FILE_EXEC_NATIVE остаются вне измеряемого интервала.
nativeEntry:
    push de
    call mx_memInfo
    ld (nativeBank),a
    ld (controlCopy+3),a
    ld (resultCopy),a
    pop hl
    or a
    jp z,nativeUsage
nativeSkipSpace:
    ld a,(hl)
    cp ' '
    jp nz,nativeParse
    inc hl
    jp nativeSkipSpace
nativeParse:
    ld de,0
    ld b,4
nativeDigit:
    ld a,(hl)
    inc hl
    sub '0'
    jp c,nativeUsage
    cp 10
    jp c,nativeNibble
    sub 7
    cp 10
    jp c,nativeUsage
    cp 16
    jp nc,nativeUsage
nativeNibble:
    push hl
    ex de,hl
    add hl,hl
    add hl,hl
    add hl,hl
    add hl,hl
    or l
    ld l,a
    ex de,hl
    pop hl
    dec b
    jp nz,nativeDigit
    ld a,(hl)
    cp 021h
    jp nc,nativeUsage
    ex de,hl
    ld (controlCopy+1),hl
    ld (resultCopy+4),hl
    ld hl,controlCopy
    call mx_memcpyFar
    ret c
    IF FONT_TEST
    ld a,(nativeBank)
    ld (bulkRequest),a
    ld (lineRequest),a
    ld (corpusCopy+3),a
    ld hl,(controlCopy+1)
    ld de,12
    add hl,de
    ld e,a
    ld b,0
    ld a,MEM_OP_PUT8
    call mx_memory
    jp c,nativeFailure
    ld hl,(nativeControl+10)
    ld (corpusCopy+1),hl
    ld hl,corpusCopy
    call mx_memcpyFar
    jp c,nativeFailure
    ld hl,(nativeControl)
    ld (lineRequest+4),hl
    ENDIF
    ld hl,(nativeControl+4)
    ld b,h
    ld c,l
    ld hl,(nativeControl+2)
    ex de,hl
    ld hl,(nativeControl)
    ld a,(nativeControl+7)
    call moduleEntry
nativeFailure:
    push af
    ld (nativeControl),hl
    ex de,hl
    ld (nativeControl+2),hl
    pop hl
    ld (nativeControl+6),hl
    ld hl,resultCopy
    call mx_memcpyFar
    ret
nativeUsage:
    ld hl,nativeUsageText
    MXOS_CALL MXOS_BIOS_PRINT_STRING_BANK
    ret
nativeBank: DB 0
nativeControl: DS 14,0
controlCopy: DB 0
    DW 0
    DB 0
    DW nativeControl,14
resultCopy: DB 0
    DW nativeControl
    DB 0
    DW 0,8
    IF FONT_TEST
corpusCopy: DB 0
    DW 0
    DB 0
    DW 02000h,1201
nativeUsageText: DB 'Run FONTBEN.COM to select tests.',13,10,0
    ELSE
nativeUsageText: DB 'Run TEXTBEN.COM to select tests.',13,10,0
    ENDIF
