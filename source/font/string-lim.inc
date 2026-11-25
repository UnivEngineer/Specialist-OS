;----------------------------------------------------------------------------
; MXOS
; Вывести строку заданной длины с обработкой управляющих кодов.
; вход: HL — буфер, BC — точное число байтов, включая управляющие коды.
; выход: HL — исключающий конец, AF/BC/DE сохранены.
; При BC=0 чтений из буфера и обращений к экрану нет.
;----------------------------------------------------------------------------
printStringLim:
    push af
    push bc
    push de
    ld a,80h
    ld (fast6.realMode),a
stringLimNext:
    ld a,b
    or c
    jp z,realStringDone
    push bc
    push hl
    call stringTryGroup
    pop de
    pop bc
    jp c,stringLimControl
    FONT_STRING_LIMIT_REMAINDER
    jp stringLimNext
stringLimControl:
    push bc
    push hl
    ld c,(hl)
    call printCharBody
    pop hl
    pop bc
    inc hl
    dec bc
    jp stringLimNext
