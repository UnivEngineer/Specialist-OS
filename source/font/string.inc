; ASCIIZ HL; AF/BC/DE сохранены, HL возвращается на завершающем нуле.
; Общий набор регистров сохраняется один раз на строку.
printString:
printStringZ:
    push af
    push bc
    push de

printString_1:
    ld a,(hl)
    or a
    jp z,printString_2
    push hl
    call fast6.tryGroup
    jp c,printStringChar
    pop de ; Старый указатель не нужен: HL уже указывает на остаток пакета.
    jp printString_1
printStringChar:
    pop hl
    ld c,(hl)
    push hl
    call printCharBody
    pop hl
    inc hl
    jp printString_1
printString_2:
    pop de
    pop bc
    pop af
    ret

; ASCIIZ без управляющих кодов: 01h–FFh — глифы, 00h завершает.
; KOI, инверсия и курсор сохраняют обычную семантику; ESC-состояние не меняется.
printStringReal:
printStringRawZ:
    push af
    push bc
    push de

    ld a,1
    ld (fast6.realMode),a
realStringNext:
    ld a,(hl)
    or a
    jp z,realStringDone
    push hl
    call fast6.tryGroup
    jp c,realStringChar
    pop de
    jp realStringNext
realStringChar:
    pop hl
    ld c,(hl)
    push hl
    call printChar_real2
    pop hl
realStringSkip:
    inc hl
    jp realStringNext
realStringDone:
    xor a
    ld (fast6.realMode),a
    jp printString_2

; HL=буфер, BC=16-битная длина; все байты — глифы, включая 00h.
; AF/BC/DE сохранены, HL=исключающий конец. Общий остаток живёт в BC,
; а не в RAM. tryGroup получает его до разрушения регистров и ограничивает
; один пакет 64 четверками (256 байт): Win20/Far256/MAIN используют одно ядро.
; 16-битное вычитание выполняется один раз на пакет, не на каждую четверку.
printStringRawLim:
    push af
    push bc
    push de

    ld a,2
    ld (fast6.realMode),a
rawLimNext:
    ld a,b
    or c
    jp z,realStringDone
    push bc
    push hl
    call fast6.tryGroup
    pop de ; DE=начало пакета; POP не меняет CF от tryGroup.
    pop bc ; BC=остаток перед пакетом, HL=следующий непрочитанный байт.
    jp c,rawLimChar
    ; DE = новый HL - прежний HL (фактически напечатанные байты).
    ; Пакет может закончиться раньше квоты из-за границы/координат экрана.
    ; Сам HL остаётся указателем текста, BC -= DE без временной RAM.
    ld a,l
    sub e
    ld e,a
    ld a,h
    sbc a,d
    ld d,a
    ld a,c
    sub e
    ld c,a
    ld a,b
    sbc a,d
    ld b,a
    jp rawLimNext
rawLimChar:
    ; Невыровненная голова, хвост 1–3 байта и альтернативный шрифт.
    ; Скаляр разрушает регистры; сохраняем указатель и общий остаток.
    ; DEC BC на 8080 не меняет флаги: нуль проверяется OR B/C наверху.
    push bc
    push hl
    ld c,(hl)
    call printChar_real2
    pop hl
    pop bc
    inc hl
    dec bc
    jp rawLimNext
