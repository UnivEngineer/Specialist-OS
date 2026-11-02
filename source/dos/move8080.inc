; Общий memmove для 8080. HL=начало, DE=конец исключительно, BC=назначение.
; Диапазоны без оборота адреса; DE=0 допускает конец 10000h, кроме полного 64 КБ.
; Выход HL=исходный конец, BC=конец назначения; AF/DE/флаги разрушены.
; Стек и выполняемый код не должны входить в переносимые области.
; Нереентерабелен, прерывания запрещены контрактом MXOS: SP временно служит данным.
moveCore:
            push    de
            ld      a, e
            sub     l
            ld      e, a
            ld      a, d
            sbc     a, h
            ld      d, a
            or      e
            jp z,   moveEmpty
            push    bc
            push    hl
            ex      de, hl
            ld      b, h
            ld      c, l
            pop     hl
            pop     de
            ; BC=число, HL=источник, DE=назначение.
            ld      a, d
            cp      h
            jp c,   moveForward
            jp nz,  moveBackward
            ld      a, e
            cp      l
            jp c,   moveForward
moveBackward:
            add     hl, bc
            ex      de, hl
            add     hl, bc
            push    hl
            ex      de, hl
            ld      a, c
            and     1
            jp z,   moveBackEven
            dec     hl
            dec     de
            ld      a, (hl)
            ld      (de), a
            dec     bc
moveBackEven:
            ld      a, b
            or      c
            call nz, movePairsBack
            pop     bc
            pop     hl
            ret
moveForward:
            ld      a, c
            and     1
            jp z,   moveForwardEven
            ld      a, (hl)
            ld      (de), a
            inc     hl
            inc     de
            dec     bc
moveForwardEven:
            ex      de, hl
            ld      a, b
            or      c
            call nz, movePairsForward
            ld      b, h
            ld      c, l
            pop     hl
            ret
moveEmpty:
            pop     hl
            ret

; Пара вперёд: 63 такта / 2 байта; назад: 64 / 2 байта.
; На пару одно чтение и одна запись каждого байта, без промежуточного буфера.
; Вход BC=чётное ненулевое число; вперёд HL=назначение, DE=источник,
; назад HL=конец источника, DE=конец назначения. Разрушает AF/BC/DE.
movePairsForward:
            ex      de, hl
            ld      (moveReadSP+1), hl
            ld      hl, 0
            add     hl, sp
            ld      (moveRestoreSP+1), hl
            ex      de, hl
moveReadSP: ld      sp, 0
movePairForwardLoop:
            pop     de
            ld      (hl), e
            inc     hl
            ld      (hl), d
            inc     hl
            dec     bc
            dec     bc
            ld      a, b
            or      c
            jp nz,  movePairForwardLoop
            jp      moveRestoreSP
movePairsBack:
            ex      de, hl
            ld      (moveWriteSP+1), hl
            ld      hl, 0
            add     hl, sp
            ld      (moveRestoreSP+1), hl
            ex      de, hl
moveWriteSP: ld      sp, 0
movePairBackLoop:
            dec     hl
            ld      d, (hl)
            dec     hl
            ld      e, (hl)
            push    de
            dec     bc
            dec     bc
            ld      a, b
            or      c
            jp nz,  movePairBackLoop
moveRestoreSP:
            ld      sp, 0
            ret
moveCoreEnd:
