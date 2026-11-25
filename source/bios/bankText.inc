; Исполнитель TEXT в MAIN. Окно остаётся источником на всё время печати.
    MODULE bankText
window:
    or a
    jp z,bios_printStringZ
    jp bios_printStringRawZ
;----------------------------------------------------------------------------
; MXOS
; Печать порции без копирования. До возврата движок не занимает окно.
; вход: HL = BANK_WINDOW, BC = длина до 32, A = D = режим MEM.
; выход: C сохраняется.
;----------------------------------------------------------------------------
printWindow:
    cp MEM_TEXT_RAW_LIM
    jp z,bios_printStringRawLim
    jp c,printKnown
    cp MEM_TEXT_LIM
    jp z,bios_printStringLim
    jp bios_printStringRawPseudo
printKnown:
    or a
    ld hl,bios_printStringZ
    ld de,FONT_ENGINE_Z+1
    jp z,windowVector
    ld hl,bios_printStringRawZ
    ld de,FONT_ENGINE_RAWZ+1
windowVector:
    ld b,a
    call fontVectorMatches
    jp nz,windowLegacy
    ld a,b
    or a
    ld b,0
    ld hl,BANK_WINDOW
    jp nz,FONT_ENGINE_LIM
    call textLimCapability
    ld hl,BANK_WINDOW
    jp z,FONT_ENGINE_TEXT_LIM
windowLegacy:
    ld d,b
    ld hl,BANK_WINDOW
    ld a,c
    cp BANK_TEXT_BYTES
    ld a,d
    jp nz,window
    ld a,(BANK_WINDOW+BANK_TEXT_BYTES-1)
    ld e,a
    xor a
    ld (BANK_WINDOW+BANK_TEXT_BYTES-1),a
    ld a,d
    call window
    dec hl
    ld b,(hl)
    ld (hl),e
    ld a,d
    call window
    ld (hl),e
    dec hl
    ld (hl),b
    ret

; HL — публичный вектор, DE — адрес операнда штатного JMP движка.
; Z=1 только при совпадении самого JMP и обоих байтов операнда.
fontVectorMatches:
    ld a,(hl)
    cp 0C3h
    ret nz
    inc hl
    ld a,(de)
    cp (hl)
    ret nz
    inc de
    inc hl
    ld a,(de)
    cp (hl)
    ret
textLimCapability:
    ld hl,(FONT_ENGINE_TEXT_EXT)
    ld de,-FONT_ENGINE_TEXT_LIM
    add hl,de
    ld a,h
    or l
    ret

;----------------------------------------------------------------------------
; MXOS
; Печать ровно BC байт с управляющими кодами через активный шрифт.
; вход: HL — буфер MAIN, BC — точная длина (00h тоже управляющий код).
; выход: HL — исключающий конец, AF/BC/DE сохранены.
;----------------------------------------------------------------------------
printStringLim:
    push af
    push de
    push hl
    ld hl,bios_printChar
    ld de,FONT_ENGINE_CHAR+1
    call fontVectorMatches
    call z,textLimCapability
    pop hl
    pop de
    jp nz,printStringLimLegacy
    pop af
    jp FONT_ENGINE_TEXT_LIM
printStringLimLegacy:
    pop af
    jp BIOS_BANK_GATE_ADDR+14h
    ENDMODULE
