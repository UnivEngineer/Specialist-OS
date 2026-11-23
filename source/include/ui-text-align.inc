; Pure positioning API, usable in MAIN or a bank. No BIOS memory access.
; HL=ASCIIZ, B=left and C=width in units of two pixels, D=glyph width in pixels,
; E=baseline, A=UI_ALIGN_LEFT/CENTER/RIGHT (0/1/2).
; Output DE=BIOS cursor position; HL/BC preserved. CF=string does not fit.
UI_ALIGN_LEFT   EQU 0
UI_ALIGN_CENTER EQU 1
UI_ALIGN_RIGHT  EQU 2

;----------------------------------------------------------------------------
; MXOS
;
; Рассчитать положение строки внутри текстового поля.
;
; вход:
;   hl = строка до нуля, b = левая граница, c = ширина поля (обе в единицах двух
;   пикселей). d = чётная ширина знака в пикселях, e = базовая линия. a =
;   UI_ALIGN_LEFT, CENTER или RIGHT.
;
; выход:
;   cf = 0: d = рассчитанная абсцисса. cf = 1: строка шире поля, d = левая граница. e,
;   hl и bc сохранены; a и прочие флаги меняются.
;----------------------------------------------------------------------------

uiAlignText:
    push hl
    push bc
    ld (uiAlignMode),a
    ld a,d
    rrca
    ld d,a
    ld a,0
uiAlignMeasure:
    push af
    ld a,(hl)
    or a
    jp z,uiAlignMeasured
    pop af
    add a,d
    jp c,uiAlignTooWide
    inc hl
    jp uiAlignMeasure
uiAlignMeasured:
    pop af
    ld h,a
    ld a,c
    sub h
    jp c,uiAlignTooWide
    ld h,a
    ld a,(uiAlignMode)
    or a
    ld a,0
    jp z,uiAlignOffset
    ld a,h
    push af
    ld a,(uiAlignMode)
    cp UI_ALIGN_CENTER
    jp nz,uiAlignRight
    pop af
    or a
    rra
    jp uiAlignOffset
uiAlignRight:
    pop af
uiAlignOffset:
    add a,b
    ld d,a
    pop bc
    pop hl
    or a
    ret
uiAlignTooWide:
    ld d,b
    pop bc
    pop hl
    scf
    ret
uiAlignMode: DB 0
