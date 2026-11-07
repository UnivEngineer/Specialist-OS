; Парная линковка BIOS/DOS: внутренние адреса разрешаются за одну сборку.
; Физические образы разделяет scripts/build/build-bios-dos.py.
    DEFINE DOS_REFERENCE_BUILD 1 ; Линковочная копия DOS не входит в BIOS.SYS.
    INCLUDE "../include/mxos.inc"
    INCLUDE "../common/layoutMacros.inc"
    INCLUDE "../../bin/banked/transport-imports.inc"
    INCLUDE "../../bin/System.layout.inc"
    INCLUDE "getch.inc"
    INCLUDE "printString1.inc"
    INCLUDE "cmp_hl_de_2.inc"
    END
