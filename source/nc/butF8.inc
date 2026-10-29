;----------------------------------------------------------------------------
; MXOS NC.COM
; Клавиша F8 - удаление файла
;
; 2013-12-18 Дизассемблировано vinxru
;----------------------------------------------------------------------------

but_F8:     ; Рисуем окно
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

            ; Если файл системный или только для чтения
            call        getSelectedFile         
            ld          de, FILE_DESCRIPTOR.attrib
            add         hl, de
            ld          a, (hl)
            and         005h
            jp z,       loc_D595

            call        drawWindow2

            ; Выводим предупреждение о защищённом файле
            COLOR       COLOR_DIALOG_ERR
            CENTER_LINE 24, 80h
            ld          hl, aFileProtected
            call        bios_printString

            ; Выводим vars.input
            COLOR       COLOR_DIALOG
            ld          hl, 5090h
            ld          (bios_vars.cursorY), hl         
            ld          hl, vars.input
            call        printStringInv

loc_D581:   ; Цикл ожидания нажатия любой клавиши
            call        bios_keyScan
            inc         a
            jp nz,      loc_D581

loc_D588:   ; Цикл ожидания нажатия Esc или Enter
            call        bios_keyScan
            cp          1Bh             ; Esc
            jp z,       mainReload      ; Если Esc - выход
            cp          0Dh             ; Enter
            jp nz,      loc_D588
            jp          mainReload      ; Защищённый файл не удаляется

loc_D595:   ; Удаляем текущий файл
            call        getSelectedFile
            call        bios_fileDelete
            jp c,       deleteFileError
            ; Количество файлов и курсор обновляются по реальному каталогу.
            jp          mainReload

deleteFileError:
            ld          hl, aDeleteFailed
            jp          copyFileShowError

