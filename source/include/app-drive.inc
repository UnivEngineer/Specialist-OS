; Launch drive of a DOS launcher: A = ASCII A..H; AF/BC/DE/HL may change.
; Read the resolved command before a nested execution overwrites it.
    IFNDEF APP_COMMAND_ADDR
    DEFINE APP_COMMAND_ADDR DOS_BANK_CMD_ADDR
    ENDIF

;----------------------------------------------------------------------------
; MXOS
;
; Определить диск запуска программы по сохранённой командной строке DOS.
;
; вход:
;   APP_COMMAND_ADDR указывает на разрешённое имя команды; читать до следующего
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
    MXOS_CALL MXOS_DOS_FILE_GET_SET_DRIVE
    add a,'A'
    ret
appDriveUpper:
    and 0DFh
    ret
