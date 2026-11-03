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
    ASSERT reboot == bios_restartOld
    ASSERT setKeybMode82 == bios_setKeybMode82
    ASSERT setKeybMode91 == bios_setKeybMode91
    ASSERT v_keybTbl == bios_keybTable
    IF NEW_MEMORY_MAP == 0
        ASSERT_DONT_FIT 0D000h
    ENDIF
    ASSERT_DONT_FIT CMD_LINE
    ASSERT_DONT_FIT STD_IO_ADDR
    ASSERT_DONT_FIT DOS_END_MAX
    END
