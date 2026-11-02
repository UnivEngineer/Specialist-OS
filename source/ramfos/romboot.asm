; Загрузчик самостоятельного ROM RAMFOS. Выполняется в MX-странице ПЗУ.
; Стек до входа RAMFOS не используется; переключение выполняется из ОЗУ 8000h.
    ORG 0
    jp boot
    jp boot
    jp boot
    jp boot
    jp boot
boot:
    ld hl, 0100h
    ld de, 0C000h
    ld bc, 01300h
copy1:
    ld a, (hl)
    ld (de), a
    inc hl
    inc de
    dec bc
    ld a, b
    or c
    jp nz, copy1
    ld de, 0F800h
    ld bc, 007E0h
copy2:
    ld a, (hl)
    ld (de), a
    inc hl
    inc de
    dec bc
    ld a, b
    or c
    jp nz, copy2
    ; 8000h доступен как основное ОЗУ и при включённом MX ПЗУ.
    ld hl, gate
    ld de, 08000h
    ld b, 6
copyGate:
    ld a, (hl)
    ld (de), a
    inc hl
    inc de
    dec b
    jp nz, copyGate
    jp 08000h
gate:
    ld (0FFFCh), a
    jp 0F800h
    ASSERT $ <= 0100h
