; Зафиксированная копия source/dos/driverFFC0.inc от 06.11.2026.
; Аппаратные адреса и алгоритм независимы от будущей реализации BIOS.
builtinBlockImage:
            ld      a,c
            ld      (0FFFDh),a
            ld      a,(de)
            ld      (0FFFCh),a
            ld      (hl),a
            inc     hl
            inc     e
            jp nz,  0FFC0h
            ret
            nop
builtinBlockWrite:
            ld      b,(hl)
            ld      a,c
            ld      (0FFFDh),a
            ld      a,b
            ld      (de),a
            ld      (0FFFCh),a
            inc     hl
            inc     e
            jp nz,  0FFD0h
            ret
builtinBlockEnd:
    ASSERT builtinBlockEnd-builtinBlockImage == 32
