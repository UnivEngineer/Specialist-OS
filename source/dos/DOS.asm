; MXOS: фиксированный ABI и автоматически размещаемые реализации.
; scripts/build/pack-dos.py измеряет блоки и создаёт проверяемый INCLUDE.
; Для обычной сборки используйте build: dos / build: OS.
    INCLUDE "../include/mxos.inc"
    INCLUDE "layoutMacros.inc"
    IFDEF DOS_LAYOUT
        INCLUDE DOS_LAYOUT
    ELSE
        INCLUDE "../../bin/DOS.layout.inc"
    ENDIF
    ; EQU-псевдонимы объявляются после определения их целей.
    INCLUDE "getch.inc"
    INCLUDE "printString1.inc"
    INCLUDE "cmp_hl_de_2.inc"
    ASSERT delay_l == bios_delay_l
    ASSERT delay_b == bios_delay_b
    ASSERT reboot == 0C009h
    IF NEW_MEMORY_MAP == 0
        ASSERT_DONT_FIT 0D000h
    ENDIF
    IF RAMD_BUFFERED
        ASSERT_DONT_FIT CMD_LINE
    ENDIF
    ASSERT_DONT_FIT 0DC00h
    END
