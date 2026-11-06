; Зафиксированная копия source/dos/driverFFC0.inc от 06.11.2026.
; Аппаратные адреса и алгоритм независимы от будущей реализации BIOS.
builtinByteImage: ; эта п/п копируется на FFC0h
            ld      (0FFFDh), a
            ld      c, (hl)
            ld      (0FFFCh), a
            ret

            ; осталось места: 8 байт
            nop
            nop
            nop
            nop
            nop
            nop
            nop
            nop

builtinByteWrite: ; эта п/п копируется на FFD0h
            ld      (0FFFDh), a
            ld      (hl), c
            ld      (0FFFCh), a
            ret

            ; осталось места: 8 байт
            nop
            nop
            nop
            nop
            nop
            nop
            nop
            nop

builtinByteEnd:
    ASSERT builtinByteEnd-builtinByteImage == 32
