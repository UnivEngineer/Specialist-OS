; Локальный быстрый memcpy bootstrap для неперекрывающихся диапазонов.
; Только прямой проход; BIOS и его рабочая память не используются.
; Вход HL=источник, DE=конец исключительно, BC=назначение.
; Выход HL=исходный конец, BC=конец назначения; AF/DE/флаги разрушены.
; Пустой диапазон и нечётная длина поддерживаются.
; Прерывания запрещены: SP служит указателем чтения, код меняет свои операнды.
; Источник, назначение, выполняемый код и стек не должны конфликтовать.
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

; Хвост: 54 такта на пару; блок 16 байт — 287 тактов.
; BC=чётное ненулевое число, HL=назначение, DE=источник.
; Разрушает AF/BC/DE. SP временно читает источник через POP.
movePairsForward:
            ex      de, hl
            ld      (moveReadSP+1), hl
            ld      hl, 0
            add     hl, sp
            ld      (moveRestoreSP+1), hl
            ex      de, hl
moveReadSP: ld      sp, 0
            ld      a, c
            or      a
            jp z,   moveForwardPageCheck
; Сначала чётный хвост до 254 байт, затем полные блоки по 256.
movePairForwardLoop:
            pop     de
            ld      (hl), e
            inc     hl
            ld      (hl), d
            inc     hl
            dec     c
            dec     c
            jp nz,  movePairForwardLoop
moveForwardPageCheck:
            ld      a, b
            or      a
            jp z,   moveRestoreSP
moveForwardPage:
            ld      c, 16
moveForwardBlock:
            DUP 8
            pop     de
            ld      (hl), e
            inc     hl
            ld      (hl), d
            inc     hl
            EDUP
            dec     c
            jp nz,  moveForwardBlock
            dec     b
            jp nz,  moveForwardPage
moveRestoreSP:
            ld      sp, 0
            ret
moveCoreEnd:

