; Стандартный BIOS PRINT_STRING получает то же 32-байтное окно.
; Вход уже проверен CLIENT; исходный пакет остаётся на стеке для bankMxResult.
memBiosRoute:
    ld a,c
    cp MXOS_BIOS_COMMAND_BATCH & 255
    jp z,memBiosQueue
    ld a,c
    cp MXOS_BIOS_PRINT_STRING_Z & 0FFh
    ld a,0
    jp z,memBiosText
    ld a,c
    cp MXOS_BIOS_PRINT_STRING_RAW_Z & 0FFh
    ld a,1
    jp z,memBiosText
    ld a,c
    cp MXOS_BIOS_PRINT_STRING_RAW_LIM & 0FFh
    ld a,2
    jp z,memBiosText
    ld a,c
    cp MXOS_BIOS_PRINT_STRING_RAW_PSEUDO & 255
    ld a,8
    jp z,memBiosText
memBiosGeneric:
    xor a
    ld hl,BIOS_BANK_GATE_ADDR+3
    jp bankMxInvoke
memBiosText:
    ld (memOperation),a
memBiosString:
    ; Последовательное чтение MX_REGS; EX (SP),HL сохраняет исходный AF.
    ex de,hl
    ld e,(hl)
    inc hl
    ld d,(hl)
    inc hl
    push de
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
    ex (sp),hl
    call memPrintTyped
    jp c,bankBiosStringError
    xor a
    ld (BANK_FRAME_STATUS),a
    pop af
    jp bankMxResult
