; Общий F806h принимает выход в CPU. Временный входной MX_REGS остаётся в стеке.
bankMxRegsRoute:
    ld a,(bankPage)
    or a
    jp z,bankMxNativeRegs
    ld a,b
    cp MXOS_GROUP_DOS
    jp z,bankMxRegsDos
    ; Очередь уже сброшена. Пачка проходит прежний пакетный маршрут.
    ld a,c
    cp MXOS_BIOS_COMMAND_BATCH & 255
    jp z,bankMxCall
    ; BIOS TEXT уже обработан. Для другой группы совпадение номера
    ; сохраняет прежний пакетный отказ.
    call memBiosTextMode
    jp z,bankMxCall
    xor a
    ld hl,BIOS_BANK_GATE_ADDR+3
    jp bankCallPacket
; HL указывает на DW, в стеке исходные AF и HL. Меняется только HL результата.
bankFastCheckText:
    call memBiosTextMode
    jp nz,bankFastFallback
    ld (memOperation),a
    inc hl
    inc hl
    pop af
    ex (sp),hl
    push af
    call memPrintTyped
    pop af
    ret
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
