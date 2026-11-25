; Native BIOS API grid. Timing and corpus preparation are local to this bank.
    INCLUDE "grid-corpus.inc"
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
    call mx_getCursorPos
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
    ld hl,(GRID_BANK_SCRATCH) ; Буфер отчёта MAIN до memTop.
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
    ld hl,GRID_BANK_CORPUS
    ld b,GRID_TEXT_BYTES/GRID_UNROLL
; Каждый байт проходит через фиксированный регистровый вход своего обработчика.
; Развёртка лишь убирает лишние расходы управляющего цикла стенда.
gridBankCharLoop:
    DUP GRID_UNROLL
    ld c,(hl)
    call mx_printChar
    inc hl
    EDUP
    dec b
    jp nz,gridBankCharLoop
    jp gridBankFinish
gridBankRawChar:
    ld hl,GRID_BANK_CORPUS
    ld b,GRID_TEXT_BYTES/GRID_UNROLL
gridBankRawLoop:
    DUP GRID_UNROLL
    ld a,(hl)
    call mx_printCharRaw
    inc hl
    EDUP
    dec b
    jp nz,gridBankRawLoop
    jp gridBankFinish
gridBankString:
    ld a,(gridMethod)
    or a
    jp nz,gridBankFar
    ld hl,GRID_BANK_CORPUS
    ld bc,GRID_TEXT_BYTES
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
    ld hl,GRID_TEXT_BYTES+1
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
gridKind:           DB 0
gridMethod:         DB 0
gridAllocation:     DW 0
gridEmpty:          DS 21,0
; kind, transport, source, length, main destination, payload capacity.
gridRequest:        DB 0,0
    DW GRID_BANK_CORPUS,GRID_TEXT_BYTES+1,0,2048
