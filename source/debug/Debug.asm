;----------------------------------------------------------------------------
; MXOS - MON2.COM
;
; 2022-02-07 Дизассемблировано SpaceEngineer
;----------------------------------------------------------------------------

    INCLUDE "../include/mxos.inc"

; Используемые подпрограммы DOS.SYS:
; bios_keyScanOld      = 0C003h
; bios_drawCursorOld   = 0C006h
; bios_printCharOld    = 0C037h
; bios_beep_Old        = 0C170h
; bios_getchOld        = 0C337h
; bios_tapeReadOld     = 0C377h
; bios_tapeWriteOld    = 0C3D0h
; bios_cmp_hl_de       = 0C427h
; bios_printStringOld  = 0C438h
; bios_reboot          = 0C800h
; bios_getch           = 0C803h
; bios_printChar       = 0C809h
; bios_input           = 0C80Fh
; bios_keyCheck        = 0C812h
; bios_printHexByte    = 0C815h
; bios_printString     = 0C818h
; bios_keyScan         = 0C81Bh
; bios_getCursorPos    = 0C81Eh
; bios_tapeLoad        = 0C824h
; bios_tapeSave        = 0C827h
; bios_calcCS          = 0C82Ah
; bios_getMemTop       = 0C830h
; bios_setMemTop       = 0C833h
; bios_printer         = 0C836h
; bios_fileList        = 0C83Fh
; bios_fileGetSetDrive = 0C842h
; bios_fileCreate      = 0C845h
; bios_fileLoad        = 0C848h
; bios_fileLoadInfo    = 0C851h
; bios_fileGetSetAddr  = 0C854h
; bios_fileGetSetAttr  = 0C857h
; bios_fileNamePrepare = 0C85Ah
; bios_fileLoad2       = 0C866h

; Используемые переменные DOS.SYS:
; bios_vars.tapeError  = 8FE1h
; bios_vars.tapeAddr   = 8FE3h
; bios_vars.cursorCfg  = 8FE9h
; bios_vars.koi7       = 8FEAh
; bios_vars.cursorX    = 8FFCh

;----------------------------------------------------------------------------
    INCLUDE "state.inc" ; Рабочая память и размещение программы
    INCLUDE "command.inc" ; Приглашение, ввод и диспетчер директив
    INCLUDE "disk.inc" ; Выход, каталог и дисковые директивы
    INCLUDE "debug.inc" ; Чтение ленты, регистры и программная точка останова
    INCLUDE "memory.inc" ; Поиск, сравнение, перенос, заполнение и текстовый дамп
    INCLUDE "input.inc" ; Разбор HEX-параметров и вход ввода строки
    INCLUDE "console.inc" ; Редактор памяти, ввод HEX, пауза и HEX-дамп
    INCLUDE "legacy.inc" ; Остатки старого обмена и общий ввод строки
    INCLUDE "vectors.inc" ; Фиксированная таблица совместимости F800h

    END
