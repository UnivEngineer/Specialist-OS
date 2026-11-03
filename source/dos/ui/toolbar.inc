; Строка F1–F8: HL = ASCII-строка до 0, B = цвет клавиш, C = цвет подписей.
; Инверсия BIOS должна быть выключена. Экран не очищается.
; AF/BC/DE/HL и флаги разрушаются, координаты BIOS меняются.
uiToolbar:
            push    bc
    IF ENABLE_COLOR
            ld      a, b
            ld      (IO_COLOR), a
    ENDIF
            push    hl
            ld      hl, 01FFh
            ld      (bios_vars.cursorY), hl
            pop     hl
            call    bios_printString
            pop     bc
    IF ENABLE_COLOR
            ld      a, c
            ld      (IO_COLOR), a
    ENDIF
            ld      hl, 92F7h
            ld      d, 8
uiToolbarLoop:
            ld      bc, (4 << 8) + 9
            call    uiInverseRect
            inc     h
            inc     h
            dec     d
            jp nz,  uiToolbarLoop
            ret
