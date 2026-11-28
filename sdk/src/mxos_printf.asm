; Консольный printf для sccz80. Форматы разбирает штатный asm_printf z88dk.
; Локальный буфер позволяет печатать результат любой длины за один проход.
    SECTION code_clib
    INCLUDE "mxos_syscall_ids.inc"
    PUBLIC printf, _mxos_printf_buffer
    EXTERN asm_printf, _mxos_home_page

    defc PRINTF_DEFAULT_BYTES = 256
    defc PRINTF_MAIN_END_HI   = 08Ch

; На входе A — число 16-битных слов аргументов sccz80, формат лежит в SP+2*A.
; На выходе HL — число выданных байтов или -1 при ошибке вывода.
printf:
    ld l,a
    ld h,0
    add hl,hl
    add hl,sp
    xor a
    ld (printfError),a
    ld bc,0                     ; Контекст приёмника.
    push bc
    ld bc,printfByte
    push bc
    ld bc,1                     ; Аргументы sccz80 идут в сторону младших адресов.
    push bc
    ld e,(hl)
    inc hl
    ld d,(hl)
    push de                     ; Формат.
    dec hl
    dec hl
    dec hl
    push hl                     ; Первый аргумент после формата.
    ld hl,(printfSource)
    ld (printfCursor),hl
    call asm_printf
    pop bc
    pop bc
    pop bc
    pop bc
    pop bc
    push hl
    ld hl,(printfSource)
    ex de,hl
    ld hl,(printfCursor)
    ld a,l
    sub e
    ld c,a
    ld a,h
    sbc a,d
    ld b,a
    or c
    jp z,printfResult
    call printfFlush
printfResult:
    pop hl
    ld a,(printfError)
    or a
    ret z
    ld hl,-1
    ret

; Callback callee-cleanup: SP+2 — контекст, SP+4 — байт форматтера.
printfByte:
    pop hl
    pop bc
    pop de
    push hl
    ld a,(printfError)
    or a
    ret nz
    ld hl,(printfCursor)
    ld (hl),e
    inc hl
    ld (printfCursor),hl
    ld a,(printfEnd)
    cp l
    ret nz
    ld a,(printfEnd+1)
    cp h
    ret nz
    ld hl,(printfCapacity)
    ld b,h
    ld c,l
    ld hl,(printfSource)
    ld (printfCursor),hl
printfFlush:
    ld hl,(printfSource)
    ld a,(_mxos_home_page)
    or a
    jp z,printfNear
    ld d,a
    ld a,(printfBound)
    or a                        ; StringLim сохраняет входные флаги.
    jp nz,printfFar
printfNear:
    call mx_callRegs
    defw MXOS_BIOS_PRINT_STRING_LIM
    jp printfStatus
printfFar:
    ld a,d
    ld (printfSourcePage),a
    ; До записи проверить всю привязанную область приложения в MAIN.
    ld hl,(printfMain)
    ld a,h
    or a
    jp z,printfFailed
    ld de,(printfCapacity)
    add hl,de
    jp c,printfFailed
    ld a,h
    cp PRINTF_MAIN_END_HI
    jp c,printfRangeOK
    jp nz,printfFailed
    ld a,l
    or a
    jp nz,printfFailed
printfRangeOK:
    ld h,b
    ld l,c
    ld (printfCount),hl
    ld hl,printfCopy
    call mx_memcpyFar
    jp c,printfFailed
    ld hl,(printfMain)
    call mx_callRegs
    defw MXOS_BIOS_PRINT_STRING_LIM_MAIN
printfStatus:
    ret nc
printfFailed:
    ld a,1
    ld (printfError),a
    ret

; Стек sccz80: адрес возврата, ёмкость, адрес MAIN и локальный адрес.
_mxos_printf_buffer:
    ld hl,2
    add hl,sp
    ld e,(hl)
    inc hl
    ld d,(hl)
    inc hl
    ld c,(hl)
    inc hl
    ld b,(hl)
    inc hl
    ld a,(hl)
    inc hl
    ld h,(hl)
    ld l,a
    push hl
    ld h,b
    ld l,c
    ld (printfMain),hl
    pop hl
    ld a,h
    or l
    jp z,printfDefault
    ld a,d
    or e
    jp z,printfDefault
    ld a,1
    jp printfSetBuffers
printfDefault:
    ld hl,printfText
    ld de,PRINTF_DEFAULT_BYTES
    xor a
printfSetBuffers:
    ld (printfBound),a
    ld (printfSource),hl
    add hl,de
    ld (printfEnd),hl
    ex de,hl
    ld (printfCapacity),hl
    ret

    SECTION data_clib
printfBound:      defb 0
printfCapacity:   defw PRINTF_DEFAULT_BYTES
printfEnd:        defw printfText+PRINTF_DEFAULT_BYTES

printfCopy:
printfSourcePage: defb 0
printfSource:     defw printfText
                  defb 0                ; Банк назначения MAIN.
printfMain:       defw 0
printfCount:      defw 0

    SECTION bss_clib
printfCursor:     defs 2
printfError:      defs 1
printfText:       defs PRINTF_DEFAULT_BYTES
