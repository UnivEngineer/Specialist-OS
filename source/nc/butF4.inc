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
        ; Запуск редактора
        ld      de, aEditor   ; "A:E.COM\r"
        jp      Enter_5


; E.COM без имени открывает новый пустой документ.
ncNewFile:
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
        jp EnterExec
