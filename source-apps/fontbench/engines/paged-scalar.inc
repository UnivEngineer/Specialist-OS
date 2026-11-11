; Local copy of the unrolled scalar 6x8 pixel kernel.
; Развёрнутый 6x8 для одиночного символа и невыровненных краёв.
; BC — экран, DE — глиф, H — следующий X/2; регистры разрушены.
    IFDEF SCALAR6_PAGED
    MODULE scalar6Paged
    ELSE
    MODULE scalar6
    ENDIF
scalar6Draw:
            ld a, h
            and 3
            ex de, hl
            ld e, a
            ld a, (bios_vars.inverse)
            or a
            jp z, inverseReady
            ld a, 3Fh
inverseReady:
            ld d, a
            ld a, e
            or a
            jp z, phase0
            dec a
            jp z, phase1
            dec a
            jp z, phase2
            jp phase3

; Одноколоночный вывод. E — временный байт экрана, D — маска инверсии.
    IFNDEF SCALAR6_ONE_DEFINED
    DEFINE SCALAR6_ONE_DEFINED
    MACRO ONE_ROW shifts, keep, advance
            ld a, (bc)
            and keep
            ld e, a
            ld a, (hl)
            xor d
            IF shifts
            DUP shifts
            add a,a
            EDUP
            ENDIF
            xor e
            ld (bc), a
            IF advance
            IFDEF SCALAR6_PAGED
            inc h
            ELSE
            inc hl
            ENDIF
            inc bc
            ENDIF
    ENDM
    ENDIF
phase0:
            DUP 7
            ONE_ROW 0, 0C0h, 1
            EDUP
            ONE_ROW 0, 0C0h, 0
            ret
phase3:
            DUP 7
            ONE_ROW 2, 3, 1
            EDUP
            ONE_ROW 2, 3, 0
            ret

; E — один повёрнутый байт глифа для обеих колонок.
; (old XOR glyph) AND keep XOR glyph сохраняет соседние пиксели
; без PUSH/POP и без повторного чтения/вращения строки знакогенератора.
    IFNDEF SCALAR6_TWO_DEFINED
    DEFINE SCALAR6_TWO_DEFINED
    MACRO TWO_ROW shifts, keepLeft, keepRight, advance
            ld a, (hl)
            xor d
            DUP shifts
            rrca
            EDUP
            ld e, a
            ld a, (bc)
            xor e
            and keepLeft
            xor e
            ld (bc), a
            inc b
            ld a, (bc)
            xor e
            and keepRight
            xor e
            ld (bc), a
            IF advance
            dec b
            IFDEF SCALAR6_PAGED
            inc h
            ELSE
            inc hl
            ENDIF
            inc bc
            ENDIF
    ENDM
    ENDIF
phase1:
            DUP 7
            TWO_ROW 2, 0F0h, 3Fh, 1
            EDUP
            TWO_ROW 2, 0F0h, 3Fh, 0
            ret
phase2:
            DUP 7
            TWO_ROW 4, 0FCh, 0Fh, 1
            EDUP
            TWO_ROW 4, 0FCh, 0Fh, 0
            ret

    ENDMODULE
