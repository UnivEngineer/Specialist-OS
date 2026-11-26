;----------------------------------------------------------------------------
; MXOS
;
; Определить диск запуска программы по сохранённой командной строке DOS.
;
; вход:
;   До INCLUDE задать DEFINE APP_COMMAND_ADDR CMD_LINE для MAIN
;   или передать адрес в своём банке. Адрес указывает на разрешённое имя команды; читать до следующего
;   вложенного запуска.
;
; выход:
;   a = буква A..H в ASCII. При отсутствии префикса взят текущий диск DOS. bc, de, hl и
;   флаги могут измениться.
;----------------------------------------------------------------------------

appProgramDrive:
    ld a,(APP_COMMAND_ADDR+1)
    cp ':'
    ld a,(APP_COMMAND_ADDR)
    jp z,appDriveUpper
    ld e,0
    MXOS_CALL_REGS MXOS_DOS_FILE_GET_SET_DRIVE
    add a,'A'
    ret
appDriveUpper:
    and 0DFh
    ret
