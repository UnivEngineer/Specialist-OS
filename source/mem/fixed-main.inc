memFixedMain = MAIN_FIXED_TARGET
memPrintNear:
    call bankTextCopyPartial
    jp memPrintFilled
; Возврат TEXT начинает запись в маппер, затем использует стек клиента.
; Вход: A — обратный селектор, DE — SP клиента, C — выведено байтов.
    ASSERT $ <= MEM_TEXT_RETURN
    IF $ < MEM_TEXT_RETURN
    DS MEM_TEXT_RETURN-$,0
    ENDIF
memPrintReturn:
    ld (BANK_IO_RAM),a
    ex de,hl
    ld sp,hl
    pop hl
    ld (bankServiceSP),hl
    pop de
    pop hl
    ld b,0
    add hl,bc
    ld a,c
    cp BANK_TEXT_BYTES
    jp nz,memPrintShort
    ex (sp),hl
    ld a,l
    sub c
    ld c,a
    ld a,h
    sbc a,0
    ld b,a
    pop hl
    jp memPrintLoop
memPrintShort:
    pop bc
memPrintHaveHL:
    pop de
    pop bc
    xor a
    ret
memPrintStack:
    call bankTextCopy32
    ld c,BANK_TEXT_BYTES
    jp memPrintFilled
    ASSERT $ <= MEM_SCREEN_RETURN
    DS MEM_SCREEN_RETURN-$,0
    ld (BANK_IO_RAM),a
    ld sp,hl
    ex de,hl
    pop af
    pop bc
    pop de
    ret

; Выделенные входы сохраняют DE в исходном банке. Вектор идёт через маппер
; в DE, а аргументы C, A и HL остаются на своих местах.
    MACRO MEM_BIOS_VECTOR entry, vector
entry:
    push de
    ld de,vector
    jp memFixedEnter
    ENDM
    MEM_BIOS_VECTOR memFixedPrint,bios_printChar
    MEM_BIOS_VECTOR memFixedRaw,bios_printCharRaw
    MEM_BIOS_VECTOR memFixedHexByte,bios_printHexByte
