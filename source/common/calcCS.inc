; Штатная сумма Monitor-2, HL..DE включительно; BC=сумма.
; HL=DE на выходе, DE сохраняется, AF совпадает с последним ADD A,C.
; Четыре байта на блок, сравнение границ один раз на блок.
; 54 байта. Блок вне последней страницы: 203 такта/4 байта;
; одинарный путь последней страницы: 86 тактов/байт, финальный — 64.
; В блоке 8 чтений ОЗУ, без записи данных, экран/порты не затрагиваются.
; На границе страницы остаток обрабатывается по одному байту.
calcCS:
    ld bc,0
calcCS_loop:
    ld a,h
    cp d
    jp z,calcCS_checkLast
    ld a,l
    cp 0FDh
    jp nc,calcCS_single
    ld a,(hl)
    add a,c
    ld c,a
    ld a,b
    adc a,(hl)
    ld b,a
    inc hl
    ld a,(hl)
    add a,c
    ld c,a
    ld a,b
    adc a,(hl)
    ld b,a
    inc hl
    ld a,(hl)
    add a,c
    ld c,a
    ld a,b
    adc a,(hl)
    ld b,a
    inc hl
calcCS_single:
    ld a,(hl)
    add a,c
    ld c,a
    ld a,b
    adc a,(hl)
    ld b,a
    inc hl
    jp calcCS_loop
calcCS_checkLast:
    ld a,l
    cp e
    jp nz,calcCS_single
    ld a,(hl)
    add a,c
    ld c,a
    ret
