; Парная линковка BIOS/DOS: внутренние адреса разрешаются за одну сборку.
; Физические образы разделяет scripts/build/build-bios-dos.py.
    INCLUDE "../include/mxos.inc"
    INCLUDE "../dos/layoutMacros.inc"
    INCLUDE "../../bin/banked/transport-imports.inc"
    INCLUDE "../../bin/System.layout.inc"
    INCLUDE "../dos/getch.inc"
    INCLUDE "../dos/printString1.inc"
    INCLUDE "../dos/cmp_hl_de_2.inc"
    END
