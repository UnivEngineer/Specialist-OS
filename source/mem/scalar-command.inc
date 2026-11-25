; Постоянный COMMAND-вход, в том числе в DATA. Чужой стек не используется.
; KIND: бит 7 — чтение, бит 6 — слово. DE=адрес, BC=значение.
; KIND=0 передаёт запрос полному транспорту, KIND=1 возвращает результат.
scalarCommandLanding:
    ld (hl),a                   ; Один и тот же PC в каждом установленном хвосте.
    ld a,(BANK_SCALAR_KIND)
    cp 3
    jp c,bankLanding
    ex de,hl
    rla                    ; CY=чтение; bit7=слово, иначе байт.
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
    ld (hl),a
    jp MEM_SCALAR_RETURN_ENTRY
scalarCommandEnd:
    ASSERT scalarCommandEnd <= 0FF00h
