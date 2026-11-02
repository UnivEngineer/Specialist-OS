; Перенос с перекрытием: HL=начало, DE=конец исключительно, BC=назначение.
; HL/DE/BC сохраняются, AF/флаги разрушаются. Пустой диапазон допустим.
memmove_bc_hl:
            push    hl
            push    bc
            push    de
            call    moveCore
            pop     de
            pop     bc
            pop     hl
            ret
