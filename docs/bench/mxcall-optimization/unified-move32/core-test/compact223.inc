; Экспериментальное разрывное ядро, прямые пары читаются через SP.
; Обе секции требуют памяти с разрешённой записью, прерывания выключены.
; Код и стек находятся вне переносимых диапазонов, диапазоны без оборота.

    MACRO compact223Word
        pop     de
        ld      (hl), e
        inc     hl
        ld      (hl), d
        inc     hl
    ENDM

; HL = 2 после сохранения источника на стеке.
    MACRO compact223PrepareSavedStack
        ld      hl, 2
        add     hl, sp
        ld      (compact223RestoreSP + 1), hl
        pop     hl
        ld      sp, hl
        ex      de, hl
    ENDM

    MACRO compact223Upper
compact223UpperStart:

; HL = источник, DE = конец источника, BC = назначение.
; Возвращает HL = конец источника, BC = конец назначения.
moveCore:
        ld      a, e
        sub     l
        ld      e, a
        ld      a, d
        sbc     a, h
        ld      d, a
        push    bc
        ld      b, d
        ld      c, e
        pop     de

; HL = источник, DE = назначение, BC = длина. Выход как у moveCore.
moveCoreCount:
        ld      a, e
        sub     l
        ld      a, d
        sbc     a, h
        jp c,   moveBytesForward
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

; HL = источник, DE = назначение, BC = длина. Выход как у moveCore.
moveBytesForward:
        push    hl
        add     hl, bc
        ex      (sp), hl
        call    compact223Forward
        pop     hl
        ret

; HL = источник, DE = назначение, C = 0..31. B игнорируется.
; Возвращает BC = конец назначения. HL, DE и AF меняются.
moveTransportForward:
        ld      b, 0
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
        add     a, low(compact223BlockEnd + 5)
        ld      (compact223Partial + 1), a
        ld      a, c
        rlca
        rlca
        rlca
        and     7
        ld      c, a
        inc     c
        inc     b
        compact223PrepareSavedStack
compact223Partial:
        jp      compact223BlockEnd

; HL = источник, DE = назначение. BC игнорируется.
; Копирует ровно 32 байта. Выход как у moveTransportForward.
moveTransportBlock32:
        ld      bc, 0101h
        push    hl
        ld      hl, 2
        jp      compact223BlockFast
compact223UpperEnd:
    ENDM

    MACRO compact223Hot
compact223HotStart:
compact223Page:
        ld      c, 8
        jp      compact223Block
compact223BlockFast:
        add     hl, sp
        ld      (compact223RestoreSP + 1), hl
        pop     hl
        ld      sp, hl
        ex      de, hl
compact223Block:
        DUP 16
            compact223Word
        EDUP
compact223BlockEnd:
        dec     c
        jp nz,  compact223Block
        dec     b
        jp nz,  compact223Page
compact223RestoreSP:
        ld      sp, 0
        ld      b, h
        ld      c, l
        ret
compact223HotEnd:
        ASSERT  high(compact223Block) == high(compact223BlockEnd)
    ENDM
