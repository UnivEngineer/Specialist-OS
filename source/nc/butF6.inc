;----------------------------------------------------------------------------
; MXOS NC.COM
; F6 - переименование файла на исходном диске.
;----------------------------------------------------------------------------

but_F6:
            call    drawWindowIfSel

            CENTER_LINE 12, 5Eh
            ld      hl, aRenameOn
            call    bios_printString
            call    printSelDrive

            call    inputForRename
            cp      1Bh
            jp z,   mainReload

            ; Поле содержит только имя. Явно выбрать диск активной панели:
            ; указать другой диск в строке переименования теперь невозможно.
            ld      hl, state.panelA_drive
            call    readBytePanel
            ld      e, 1
            call    bios_fileGetSetDrive

            ld      hl, vars.input
            ld      de, vars.tempFileDescr
            call    bios_fileNamePrepare

            call    getSelectedFile
            ld      de, vars.tempFileDescr
            call    bios_fileRename
            jp c,   renameFileError
            jp      mainReload

renameFileError:
            ld      hl, aCantRenameFile
            jp      copyFileShowError