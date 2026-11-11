; Native BIOS API grid. Timing and corpus preparation are local to this bank.
gridBank:
    ld a,c
    ld (gridKind),a
    ld a,b
    ld (gridMethod),a
    ld hl,0
    ld (gridAllocation),hl
    ld a,c
    cp 2
    jp nc,gridBankStringSetup
    ld a,b
    or a
    jp nz,gridBankSeedText
    MXOS_CALL MXOS_BIOS_GET_CURSOR_POS
    jp gridBankStart
gridBankSeedText:
    ld hl,gridEmpty
    MXOS_CALL MXOS_BIOS_PRINT_STRING_Z
    jp gridBankStart
gridBankStringSetup:
    ld a,b
    cp 2
    jp nz,gridBankStart
    ld b,0
    ld hl,07C00h
    ld de,00900h
    ld a,MEM_OP_ARENA
    MXOS_CALL MXOS_DOS_MEMORY
    ret c
    ld b,0
    ld hl,2049
    ld a,MEM_OP_ALLOC
    MXOS_CALL MXOS_DOS_MEMORY
    ret c
    ld (gridAllocation),hl
gridBankStart:
    call moduleTimerStart
    ld a,(gridKind)
    cp 2
    jp nc,gridBankString
    or a
    jp nz,gridBankRawChar
    ld hl,02000h
    ld b,72
; По-прежнему 1152 отдельных MXOS_CALL, без накопления и пакетного рендера.
; Развёртка лишь убирает лишние расходы управляющего цикла стенда.
gridBankCharLoop:
    DUP 16
    ld c,(hl)
    MXOS_CALL MXOS_BIOS_PRINT_CHAR
    inc hl
    EDUP
    dec b
    jp nz,gridBankCharLoop
    jp gridBankFinish
gridBankRawChar:
    ld hl,02000h
    ld b,72
gridBankRawLoop:
    DUP 16
    ld a,(hl)
    MXOS_CALL MXOS_BIOS_PRINT_CHAR_RAW
    inc hl
    EDUP
    dec b
    jp nz,gridBankRawLoop
    jp gridBankFinish
gridBankString:
    ld a,(gridMethod)
    or a
    jp nz,gridBankFar
    ld hl,02000h
    ld bc,1152
    ld a,(gridKind)
    sub 2
    jp z,gridBankZ
    dec a
    jp z,gridBankRawZ
    MXOS_CALL MXOS_BIOS_PRINT_STRING_RAW_LIM
    jp c,gridBankError
    jp gridBankFinish
gridBankZ:
    MXOS_CALL MXOS_BIOS_PRINT_STRING_Z
    jp c,gridBankError
    jp gridBankFinish
gridBankRawZ:
    MXOS_CALL MXOS_BIOS_PRINT_STRING_RAW_Z
    jp c,gridBankError
    jp gridBankFinish
gridBankFar:
    ld (gridRequest+1),a
    ld a,(gridKind)
    sub 2
    ld (gridRequest),a
    ld hl,1153
    cp 2
    jp nz,gridBankCount
    dec hl
gridBankCount:
    ld (gridRequest+4),hl
    ld hl,(gridAllocation)
    ld (gridRequest+6),hl
    ld hl,gridRequest
    MXOS_CALL MXOS_BIOS_TEXT_PRINT
    jp c,gridBankError
gridBankFinish:
    call moduleTimerSample
    call gridBankFree
    jp moduleTimerResult
gridBankError:
    push af
    call gridBankFree
    pop af
    ret
gridBankFree:
    ld hl,(gridAllocation)
    ld a,h
    or l
    ret z
    ld b,0
    ld a,MEM_OP_FREE
    MXOS_CALL MXOS_DOS_MEMORY
    ret
gridKind: DB 0
gridMethod: DB 0
gridAllocation: DW 0
gridEmpty: DS 21,0
; kind, transport, source, length, main destination, payload capacity.
gridRequest: DB 0,0
    DW 02000h,1153,0,2048
