;----------------------------------------------------------------------------
; MXOS NC.COM
; Клавиша F8 - удаление файла
;
; 2013-12-18 Дизассемблировано vinxru
;----------------------------------------------------------------------------

F8:         ; Рисуем окно
            call    drawWindowIfSel

            ; Выводим "DELETE FROM A:"
            CENTER_LINE 14, 5Eh
            ld          hl, aDeleteFrom
            call        bios_printString
            call        printSelDrive

            call        printInvSelFile

loc_D54C:   ; Ждем нажатия Enter или ESC
            call        bios_keyScan
            cp          1Bh
            jp z,       mainReload
            cp          0Dh
            jp nz,      loc_D54C

            ; Если файл только для чтения
            call        getSelectedFile         
            ld          de, FILE_DESCRIPTOR.attrib
            add         hl, de
            ld          a, (hl)
            and         1
            jp z,       loc_D595

            call        drawWindow2

            ; Выводим текст "FILE IS READ-ONLY"
            COLOR       COLOR_DIALOG_ERR
            CENTER_LINE 18, 80h
            ld          hl, aFileIsReanOnly
            call        bios_printString

            ; Выводим nc_vars.input
            COLOR       COLOR_DIALOG
            ld          hl, 5090h
            ld          (bios_vars.cursorY), hl         
            ld          hl, nc_vars.input
            call        printStringInv

loc_D581:   ; CODE XREF: drawWindowIfSel+379j
            call        bios_keyScan
            inc         a
            jp nz,      loc_D581

loc_D588:   ; CODE XREF: drawWindowIfSel+386j
            call        bios_keyScan
            cp          1Bh
            jp z,       mainReload
            cp          0Dh
            jp nz,      loc_D588

loc_D595:   ; Удаляем текущий файл
            call        getSelectedFile
            call        bios_fileDelete

            ; Уменьшаем счетчик файлов
            ld          hl, panelA_filesCnt
            call        readBytePanel
            dec         a
            call        writeBytePanel

            jp          mainReload

