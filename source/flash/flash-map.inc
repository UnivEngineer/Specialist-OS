; Логические чипы уплотнены: допускаются пустые гнёзда между установленными.
; DE=логический сектор, HL=буфер. DE получает физический сектор/выбор чипа.
; Разрушаются AF/C; HL и B сохраняются, входные DE вернёт обёртка драйвера.
; 21 байт, 107 тактов без CALL; одно чтение таблицы и четыре обращения к стеку.
MapSector:
    push hl
    ld a,d
    and 7
    ld c,a
    ld a,d
    rrca
    rrca
    rrca
    and 3
    add a,v_chipMap & 0FFh
    ld l,a
    ld h,v_chipMap / 256
    ld a,(hl)
    or c
    ld d,a
    pop hl
    ret
v_chipMap: DB 0,0,0,0
    ASSERT v_chipMap / 256 == (v_chipMap+3) / 256
