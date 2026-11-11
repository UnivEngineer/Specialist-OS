; Развёрнутый 6x8 для одиночного символа и невыровненных краёв.
; BC — экран, DE — глиф, H — следующий X/2; регистры разрушены.
    MODULE scalar6
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
    MACRO ONE_ROW shifts, keep, advance, step
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
step:       inc h
            inc bc
            ENDIF
    ENDM
    ENDIF
phase0:
            ONE_ROW 0, 0C0h, 1, step0_0
            ONE_ROW 0, 0C0h, 1, step0_1
            ONE_ROW 0, 0C0h, 1, step0_2
            ONE_ROW 0, 0C0h, 1, step0_3
            ONE_ROW 0, 0C0h, 1, step0_4
            ONE_ROW 0, 0C0h, 1, step0_5
            ONE_ROW 0, 0C0h, 1, step0_6
            ONE_ROW 0, 0C0h, 0, unused
            ret
phase3:
            ONE_ROW 2, 3, 1, step3_0
            ONE_ROW 2, 3, 1, step3_1
            ONE_ROW 2, 3, 1, step3_2
            ONE_ROW 2, 3, 1, step3_3
            ONE_ROW 2, 3, 1, step3_4
            ONE_ROW 2, 3, 1, step3_5
            ONE_ROW 2, 3, 1, step3_6
            ONE_ROW 2, 3, 0, unused
            ret

; E — один повёрнутый байт глифа для обеих колонок.
; (old XOR glyph) AND keep XOR glyph сохраняет соседние пиксели
; без PUSH/POP и без повторного чтения/вращения строки знакогенератора.
    IFNDEF SCALAR6_TWO_DEFINED
    DEFINE SCALAR6_TWO_DEFINED
    MACRO TWO_ROW shifts, keepLeft, keepRight, advance, step
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
step:       inc h
            inc bc
            ENDIF
    ENDM
    ENDIF
phase1:
            TWO_ROW 2, 0F0h, 3Fh, 1, step1_0
            TWO_ROW 2, 0F0h, 3Fh, 1, step1_1
            TWO_ROW 2, 0F0h, 3Fh, 1, step1_2
            TWO_ROW 2, 0F0h, 3Fh, 1, step1_3
            TWO_ROW 2, 0F0h, 3Fh, 1, step1_4
            TWO_ROW 2, 0F0h, 3Fh, 1, step1_5
            TWO_ROW 2, 0F0h, 3Fh, 1, step1_6
            TWO_ROW 2, 0F0h, 3Fh, 0, unused
            ret
phase2:
            TWO_ROW 4, 0FCh, 0Fh, 1, step2_0
            TWO_ROW 4, 0FCh, 0Fh, 1, step2_1
            TWO_ROW 4, 0FCh, 0Fh, 1, step2_2
            TWO_ROW 4, 0FCh, 0Fh, 1, step2_3
            TWO_ROW 4, 0FCh, 0Fh, 1, step2_4
            TWO_ROW 4, 0FCh, 0Fh, 1, step2_5
            TWO_ROW 4, 0FCh, 0Fh, 1, step2_6
            TWO_ROW 4, 0FCh, 0Fh, 0, unused
            ret

; A=24h (INC H, page-row font) or 23h (INC HL, linear font).
; Patch only when the layout changes, never inside the pixel loop.
selectLayout:
layoutOld: cp 0
    ret z
    push bc
    push de
    push hl
    ld (layoutOld+1),a
    ld c,a
    ld hl,steps
    ld b,28
layoutLoop:
    ld e,(hl)
    inc hl
    ld d,(hl)
    inc hl
    ld a,c
    ld (de),a
    dec b
    jp nz,layoutLoop
    pop hl
    pop de
    pop bc
    ret
steps:
    DW step0_0,step0_1,step0_2,step0_3,step0_4,step0_5,step0_6
    DW step3_0,step3_1,step3_2,step3_3,step3_4,step3_5,step3_6
    DW step1_0,step1_1,step1_2,step1_3,step1_4,step1_5,step1_6
    DW step2_0,step2_1,step2_2,step2_3,step2_4,step2_5,step2_6
    ENDMODULE
