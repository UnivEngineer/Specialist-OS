; Таблица команд: ASCIIZ и адрес обработчика, ноль завершает список.
cliCommands:
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
cliExtensions: DB 'COMEXEBAT',0
cliHelpText: DB 'DIR CD PWD PATH A: MD RD COPY MOVE REN DEL TYPE CLS ECHO REM HELP EXIT ATTRIB',0
cliDirText: DB '<DIR>',0
cliSavedDirectories: DS 16,0
cliOutput: DB 0
cliQuery: DB 0
cliAttribSet: DB 0
cliAttribClear: DB 0
cliOldDrive: DB 0
cliOldDirectory: DW 0
cliArgs: DW 0
cliTail: DW 0
cliExtPtr: DW 0
cliSearchDrive: DB 0
cliNeedExtension: DB 0
cliPathCursor: DW 0
cliMoveFlag: DB 0
cliParent: DW 0
cliNewCluster: DW 0
cliRemaining: DW 0
cliCursor: DS FILE_STREAM_CURSOR,0
cliRequest: FILE_COPY_REQUEST
cliLine: DS MAX_PATH,0
cliPath1: DS MAX_PATH,0
cliPath2: DS MAX_PATH,0
cliSearchDirectory EQU cliPath2
; Ассоциации и файловые команды выполняются последовательно.
cliIoBuffer:
cliAssociationBuffer: DS 512,0
cliLowerEnd:

cliCopyHook: DW 0
