;----------------------------------------------------------------------------
; MXOS NC.COM
; Клавиша F6 - переименование
;
; 2013-12-18 Дизассемблировано vinxru
;----------------------------------------------------------------------------

F6:         ; Рисуем окно
            call        drawWindowIfSel

            ; Выводим "REMOVE FROM A: TO"
            CENTER_LINE 23, 5Eh
            ld          hl, aRemoveFromTo
            call        bios_printString
            call        printSelDrive

            call        inputForCopyMove
            cp          1Bh
            jp z,       mainReload

            ld          hl, nc_vars.input
            ld          de, nc_vars.tempFileDescr
            call        bios_fileNamePrepare

            ld          e, 2
            call        bios_fileGetSetDrive
            ld          c, a
            ld          hl, panelA_drive
            call        readBytePanel
            cp          c
            jp nz,      loc_D47B
            call        getSelectedFile
            ld          de, nc_vars.tempFileDescr
            call        bios_fileRename
            jp          mainReload
; ---------------------------------------------------------------------------

loc_D47B:   ld          e, 1
            call        bios_fileGetSetDrive
            call        getSelectedFile
            ld          de, nc_vars.tempFileDescr

            ; копируем имя файла
            ld          c, DIR_NAME_LENGTH + 3
loc_D488:
            ld          a, (hl)
            ld          (de),a
            inc         hl
            inc         de
            dec         c
            jp nz,      loc_D488

            call        copyFileInt
            ld          hl, panelA_drive
            call        readBytePanel
            ld          e, 1
            call        bios_fileGetSetDrive
            ld          hl, nc_vars.tempFileDescr
            call        bios_fileDelete
            jp          mainReload

