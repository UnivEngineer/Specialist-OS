; Стандартный BIOS PRINT_STRING получает то же 32-байтное окно.
; Вход уже проверен CLIENT; исходный пакет остаётся на стеке для bankMxResult.
memBiosRoute:
    ld a,c
    cp MXOS_BIOS_COMMAND_BATCH & 255
    jp z,memBiosQueue
    call memBiosTextMode
    jp z,memBiosText
memBiosGeneric:
    xor a
    ld hl,BIOS_BANK_GATE_ADDR+3
    jp bankMxInvoke
; A содержит младший байт BIOS ID. При совпадении Z=1, A содержит режим.
memBiosTextMode:
    sub MXOS_BIOS_PRINT_STRING_Z & 255
    ret z
    sub MXOS_BIOS_PRINT_STRING_RAW_Z-MXOS_BIOS_PRINT_STRING_Z
    jp z,memBiosTextModeRawZ
    ASSERT MXOS_BIOS_PRINT_STRING_RAW_LIM-MXOS_BIOS_PRINT_STRING_RAW_Z == MEM_TEXT_RAW_LIM
    cp MEM_TEXT_RAW_LIM
    ret z
    ; Смежные 012Dh/012Eh дают 2/3 после вычитания общей базы.
    ASSERT MXOS_BIOS_PRINT_STRING_RAW_PSEUDO+1 == MXOS_BIOS_PRINT_STRING_LIM
    sub MXOS_BIOS_PRINT_STRING_LIM-MXOS_BIOS_PRINT_STRING_RAW_Z-MEM_TEXT_LIM
    cp MEM_TEXT_LIM
    ret z
    cp MEM_TEXT_LIM-1
    ld a,MEM_TEXT_RAW_PSEUDO
    ret
memBiosTextModeRawZ:
    ld a,MEM_TEXT_RAW_Z
    ret
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
    xor a
    ld (BANK_FRAME_STATUS),a
    pop af
    jp bankMxResult

bankMxNativeRegs:
    ld hl,(MEM_NATIVE_CALL)
    inc hl
    inc hl
    inc hl
    jp (hl)
