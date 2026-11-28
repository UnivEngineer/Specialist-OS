; Таблица команд: ASCIIZ и адрес обработчика, ноль завершает список.
cliCommands:
    IFDEF MX_FLOPPY_BOOT
    DB 'SYSROM',0
    DW cliSysRom
    DB 0
    ENDIF
    DB 'ATTRIB',0
    DW cliAttrib
    DB 4
    DB 'CD',0
    DW cliCd
    DB 5
    DB 'CHDIR',0
    DW cliCd
    DB 5
    DB 'PATH',0
    DW cliPathCommand
    DB 3
    DB 'PWD',0
    DW cliPwd
    DB 1
    DB 'DIR',0
    DW cliDir
    DB 1
    DB 'MD',0
    DW cliMkdir
    DB 0
    DB 'MKDIR',0
    DW cliMkdir
    DB 0
    DB 'RD',0
    DW cliRmdir
    DB 0
    DB 'RMDIR',0
    DW cliRmdir
    DB 0
    DB 'COPY',0
    DW cliCopy
    DB 0
    DB 'MOVE',0
    DW cliMove
    DB 0
    DB 'REN',0
    DW cliRename
    DB 0
    DB 'RENAME',0
    DW cliRename
    DB 0
    DB 'DEL',0
    DW cliDelete
    DB 0
    DB 'ERASE',0
    DW cliDelete
    DB 0
    DB 'TYPE',0
    DW cliType
    DB 1
    DB 'CLS',0
    DW cliCls
    DB 1
    DB 'ECHO',0
    DW cliEcho
    DB 1
    DB 'PAUSE',0
    DW cliPause
    DB 1
    DB 'REM',0
    DW cliSuccess
    DB 0
    DB 'HELP',0
    DW cliHelp
    DB 1
    DB 'EXIT',0
    DW cliExit
    DB 0
    DB 0
cliExtensions:      DB 'COMEXEBAT',0
cliDirText:         DB '<DIR>',0
    IFDEF BANKED_DOS_BUILD
cliPath1 = DOS_CLI_PATH1
cliPath2 = DOS_CLI_PATH2
    ELSE
cliPath1: DS MAX_PATH,0
cliPath2: DS MAX_PATH,0
    ENDIF
cliSearchDirectory EQU cliPath2
