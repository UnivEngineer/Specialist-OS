; Постоянный COMMAND-вход, в том числе в DATA. Чужой стек не используется.
; KIND=3 GET16, 4 PUT16, 5 GET8, 6 PUT8. DE=адрес, BC=значение.
; Обычные kind 0..2 передаются полному транспорту.
scalarCommandLanding:
    ld a,(BANK_SCALAR_KIND)
    cp 3
    jp c,BANK_LANDING
    ex de,hl
    cp 5
    rra                    ; CY=чтение; bit7=слово, иначе байт.
    jp nc,scalarCommandWrite
    ld c,(hl)
    or a
    jp p,scalarCommandDone
    inc hl
    ld b,(hl)
    jp scalarCommandDone
scalarCommandWrite:
    ld (hl),c
    or a
    jp p,scalarCommandDone
    inc hl
    ld (hl),b
scalarCommandDone:
    ld hl,(BANK_SCALAR_RETURN_PAIR)
    ld a,h
    ld h,0FFh
    jp BANK_SCALAR_RETURN_GATE
scalarCommandEnd:
    ASSERT scalarCommandEnd <= 0FF00h
