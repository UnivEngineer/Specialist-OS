;----------------------------------------------------------------------------
; MXOS NC.COM
; Клавиша F4 - запуск редактора
;
; 2013-12-18 Дизассемблировано vinxru
;----------------------------------------------------------------------------

but_F4:
        ld a,(ncShiftState)
        or a
        jp z,ncNewFile
        ; Получение кол-ва файлов
        ld      hl, state.panelA_filesCnt
        call    readBytePanel

        ; Если файлов нет, выходим
        and     a
        jp z,   mainLoop

        call ncRealSelection
        jp c,mainLoop
        ld de,FILE_INFO.attrib
        add hl,de
        ld a,(hl)
        and 10h
        jp nz,mainLoop
    IF NC_BANKED
ncBankEditSelected:
        call getSelectedFile
        call buildSelectedFilePath
        jp c,mainLoop
        jp ncBankEditorReady
    ELSE
        ; Запуск редактора
        ld      de,aEditor
        jp      Enter_5
    ENDIF


; EDIT.COM без имени открывает новый пустой документ.
ncNewFile:
    IF NC_BANKED
        xor a
        ld (vars.pathArg),a
ncBankEditorReady:
        call ncEditorLoading
        call ncBankFlush
        call ncStopObserver
        ld a,3
        ld (ncBankExitAction),a
        jp ncBankReturn
    ELSE
        ld hl,aEditor
        ld de,vars.cmdLine
ncNewFileCopy:
        ld a,(hl)
        cp 0Dh
        jp z,ncNewFileReady
        ld (de),a
        inc hl
        inc de
        jp ncNewFileCopy
ncNewFileReady:
        xor a
        ld (de),a
        ex de,hl
        ld (vars.cmdLineEnd),hl
        jp ncLaunchEditor
    ENDIF
