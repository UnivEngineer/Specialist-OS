; [HL,HL+BC) против [DE,DE+A), A=1..255. Диапазоны проверены отдельно.
; BC сохранён; AF/DE/HL разрушены. CF=1 при перекрытии, пустой буфер допустим.
kdOverlap:
    push af
    ld a,b
    or c
    jp z,koGood
    push hl
    add hl,bc
    call cmp_hl_de
    pop hl
    jp c,koGood
    jp z,koGood
    pop af
    push hl
    ld h,0
    ld l,a
    add hl,de
    ex de,hl
    pop hl
    jp cmp_hl_de
koGood:
    pop af
    or a
    ret
