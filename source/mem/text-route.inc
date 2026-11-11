; Стандартный BIOS PRINT_STRING получает тот же быстрый Window20.
; Вход уже проверен CLIENT; исходный пакет остаётся на стеке для bankMxResult.
memBiosRoute:
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
    ld a,(BANK_PROFILE)
    cp BANK_PROFILE_TEXT
    jp nz,memBiosGeneric
    ld a,c
    cp MXOS_BIOS_PRINT_CHAR & 0FFh
    ld hl,4
    ld a,3
    jp z,memBiosChar
    ld a,c
    cp MXOS_BIOS_PRINT_CHAR_RAW & 0FFh
    jp nz,memBiosGeneric
    ld hl,7
    ld a,4
memBiosChar:
    ld (memOperation),a
    add hl,de
    ld a,(hl)
    ld (memSelect),a
    jp memBiosString
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
    call memTextCall
    jp c,bankBiosStringError
    xor a
    ld (BANK_FRAME_STATUS),a
    pop af
    jp bankMxResult

memTextCall:
    ld a,(memOperation)
    cp 3
    jp c,memPrintTyped
    jp memTextScalar
