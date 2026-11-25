; MXOS. Локальный аналог C42Dh с выбором направления по началу диапазонов.
; Вход: HL = источник, DE = исключающий конец, BC = назначение.
; Выход: HL и BC = исключающие концы источника и назначения, SP восстановлен.
moveCore:
        push    bc
        ld      a, e
        sub     l
        ld      c, a
        ld      a, d
        sbc     a, h
        ld      b, a
        pop     de

; HL = источник, DE = назначение, BC = длина. Выход как у moveCore.
moveCoreCount:
        ld      a, e
        sub     l
        ld      a, d
        sbc     a, h
moveForwardDispatch:
        jp c,   MEM_LOCAL_FORWARD
        add     hl, bc
        ex      de, hl
        add     hl, bc
        push    hl
        ex      de, hl
        push    hl
        ld      a, c
        rrca
        jp c,   compact223BackOdd
        jp      compact223BackCheck
compact223BackPair:
        dec     hl
        dec     de
        ld      a, (hl)
        ld      (de), a
        dec     bc
compact223BackOdd:
        dec     hl
        dec     de
        ld      a, (hl)
        ld      (de), a
        dec     bc
compact223BackCheck:
        ld      a, b
        or      c
        jp nz,  compact223BackPair
        pop     hl
        pop     bc
        ret

compact223Forward:
        ld      a, c
        rrca
        jp nc,  compact223ForwardEven
        ld      a, (hl)
        ld      (de), a
        inc     hl
        inc     de
compact223ForwardEven:
        push    hl
        ld      a, c
        and     30
        rrca
        cpl
        ; A = -1 - число пар остатка. Одна пара занимает 5 байт кода.
        ld      l, a
        add     a, a
        add     a, a
        add     a, l
        add     a, low(MEM_FORWARD_BLOCK_END + 5)
        ld      (compact223Partial + 1), a
        ld      a, c
        rlca
        rlca
        rlca
        and     7
        ld      c, a
        inc     c
        inc     b
        ld      hl, 2
        add     hl, sp
        ld      (MEM_FORWARD_RESTORE + 1), hl
        pop     hl
        ld      sp, hl
        ex      de, hl
compact223Partial:
        jp      MEM_FORWARD_BLOCK_END

; При установке DATA этот участок заменяется локальным циклом.
; Полный профиль использует собственную подготовку и место до FE8Ah.
moveDataForward = compact223Forward
;----------------------------------------------------------------------------
; MXOS
; Заполнить окно читающей половиной CPM32 в текущей исполняемой странице.
; вход: HL — начало 32 доступных байтов, стек клиента активен, IRQ выключены.
; выход: окно содержит 32 байта, BC и SP сохранены, AF/DE/HL рабочие.
;----------------------------------------------------------------------------
bankTextCopy32:
    ex de,hl
    ld hl,0
    add hl,sp
    ld (MEM_FORWARD_RESTORE+1),hl
    ex de,hl
    ld sp,hl
    xor a
    jp bankCopyRead32
    DS 2,0                     ; Сохраняем границу заменяемого фрагмента DATA.
moveDataForwardEnd:
