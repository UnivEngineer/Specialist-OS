; Общий F806h принимает выход в CPU. Временный входной MX_REGS остаётся в стеке.
bankMxRegsRoute:
    ld a,(bankPage)
    or a
    jp z,bankMxNativeRegs
    ld a,b
    cp MXOS_GROUP_DOS
    jp z,bankMxRegsDos
    ; Очередь уже сброшена. Текст и короткая пачка идут прямо к банковому клиенту.
    ld a,c
    cp MXOS_BIOS_COMMAND_BATCH & 255
    jp z,bankMxCall
    cp MXOS_BIOS_PRINT_STRING_Z & 255
    jp z,bankMxCall
    cp MXOS_BIOS_PRINT_STRING_RAW_Z & 255
    jp z,bankMxCall
    cp MXOS_BIOS_PRINT_STRING_RAW_LIM & 255
    jp z,bankMxCall
    cp MXOS_BIOS_PRINT_STRING_RAW_PSEUDO & 255
    jp z,bankMxCall
    xor a
    ld hl,BIOS_BANK_GATE_ADDR+3
    jp bankCallPacket
bankFastReady:
    ; Младший байт SP для BIOS_FAST_SLOT в MAIN. Старший постоянен.
    add a,BIOS_FAST_SLOT_BIAS
    rrca
    and BIOS_FAST_SLOT_MASK
    ld (bankFastBiosSP+1),a
    inc hl
    inc hl
    pop af
    ex (sp),hl
    jp bankFastPrepare
bankMxNativeRegs:
    ld hl,(MEM_NATIVE_CALL)
    inc hl
    inc hl
    inc hl
    jp (hl)
