; Native BIOS API grid. Timing and corpus preparation are local to this bank.
    INCLUDE "grid-corpus.inc"
GRID_FAR_CAPACITY = 1024
    ASSERT GRID_TEXT_BYTES <= GRID_FAR_CAPACITY
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
    MXOS_CALL_REGS MXOS_BIOS_PRINT_STRING_Z
    jp gridBankStart
gridBankStringSetup:
    ld a,b
    or a
    jp z,gridBankStart
    ld b,0
    ld hl,(GRID_BANK_SCRATCH) ; MAIN-арена на месте буфера будущего отчёта.
    ld de,00900h
    ld a,MEM_OP_ARENA
    MXOS_CALL_REGS MXOS_DOS_MEMORY
    ret c
    ld b,0
    ld hl,GRID_FAR_CAPACITY+1 ; Последний байт зарезервирован для NUL.
    ld a,MEM_OP_ALLOC
    MXOS_CALL_REGS MXOS_DOS_MEMORY
    ret c
    ld (gridAllocation),hl
    ld (gridFarCopy+MEM_COPY_REQUEST.destinationOffset),hl
    call mx_memInfo
    ld (gridFarCopy+MEM_COPY_REQUEST.sourceBank),a
    ld hl,GRID_TEXT_BYTES+1
    ld a,(gridKind)
    cp 4
    jp nz,gridBankSetCount
    dec hl                  ; RawLim печатает только полезные байты.
gridBankSetCount:
    ld (gridFarCopy+MEM_COPY_REQUEST.count),hl
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
    jp nz,gridBankFarRun
    ld hl,GRID_BANK_CORPUS
    ld bc,GRID_TEXT_BYTES
    ld a,(gridKind)
    sub 2
    jp z,gridBankZ
    dec a
    jp z,gridBankRawZ
    MXOS_CALL_REGS MXOS_BIOS_PRINT_STRING_RAW_LIM
    jp c,gridBankError
    jp gridBankFinish
gridBankZ:
    MXOS_CALL_REGS MXOS_BIOS_PRINT_STRING_Z
    jp c,gridBankError
    jp gridBankFinish
gridBankRawZ:
    MXOS_CALL_REGS MXOS_BIOS_PRINT_STRING_RAW_Z
    jp c,gridBankError
    jp gridBankFinish
gridBankFarRun:
    call gridBankFar
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
    MXOS_CALL_REGS MXOS_DOS_MEMORY
    ret

;----------------------------------------------------------------------------
; MXOS
; Перенести одну порцию в собственный буфер MAIN и напечатать её.
; вход:
;   gridKind = 2 для StringZ, 3 для RawZ, 4 для RawLim. gridFarCopy указывает
;   на корпус в текущем банке и выделенный буфер MAIN. count включает NUL
;   для Z и только полезные байты для RawLim. Полезных байтов не более 1024.
; выход:
;   CF=0, HL = адрес NUL или исключающий конец в MAIN. При отказе CF=1,
;   A = код ошибки. Остальные регистры и флаги разрушаются, буфер живёт
;   до gridBankFree. При отказе memcpy_far вывода нет.
;----------------------------------------------------------------------------
gridBankFar:
    ld hl,gridFarCopy
    call mx_memcpyFar
    ret c
    ld hl,(gridFarCopy+MEM_COPY_REQUEST.destinationOffset)
    ld a,(gridKind)
    sub 2
    jp z,gridBankFarZ
    dec a
    jp z,gridBankFarRawZ
    ex de,hl
    ld hl,(gridFarCopy+MEM_COPY_REQUEST.count)
    ld b,h
    ld c,l
    ex de,hl
    MXOS_CALL_REGS MXOS_BIOS_PRINT_STRING_RAW_LIM_MAIN
    ret
gridBankFarZ:
    MXOS_CALL_REGS MXOS_BIOS_PRINT_STRING_Z_MAIN
    ret
gridBankFarRawZ:
    MXOS_CALL_REGS MXOS_BIOS_PRINT_STRING_RAW_Z_MAIN
    ret

gridKind:           DB 0
gridMethod:         DB 0
gridAllocation:     DW 0
gridEmpty:          DS 21,0
gridFarCopy:        DB 0
                    DW GRID_BANK_CORPUS
                    DB 0
                    DW 0,0
