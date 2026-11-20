; F806 uses the CPU result. Its temporary input packet is never rewritten.
bankMxRegsRoute:
    call bankQueueBefore
    ld a,(bankPage)
    or a
    jp z,bankMxNativeRegs
    ld a,b
    cp MXOS_GROUP_DOS
    jp z,bankMxRegsDos
    ; Strings retain the existing Window20 delivery until queued.
    ld a,c
    cp MXOS_BIOS_COMMAND_BATCH & 255
    jp z,mx_call
    cp MXOS_BIOS_PRINT_STRING_Z & 255
    jp z,mx_call
    cp MXOS_BIOS_PRINT_STRING_RAW_Z & 255
    jp z,mx_call
    cp MXOS_BIOS_PRINT_STRING_RAW_LIM & 255
    jp z,mx_call
    cp MXOS_BIOS_PRINT_STRING_RAW_PSEUDO & 255
    jp z,mx_call
    xor a
    ld hl,BIOS_BANK_GATE_ADDR+3
    jp bankCallPacket
bankMxNativeRegs:
    ld hl,(MEM_NATIVE_CALL)
    inc hl
    inc hl
    inc hl
    jp (hl)
