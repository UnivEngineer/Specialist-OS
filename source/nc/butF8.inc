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
            call        bios_printStringReal
            call        printSelDrive

            call        printInvSelFile

loc_D54C:   ; Ждем нажатия Enter или ESC
            call        bios_keyScan
            cp          1Bh
            jp z,       ncRefreshDirty
            cp          0Dh
            jp nz,      loc_D54C

loc_D595:   ; Удаляем текущий файл
            ld          hl, state.panelA_drive
            call        readBytePanel
            MXOS_CALL MXOS_DOS_FILE_SELECT_DRIVE
            jp c,       deleteFileError
            call        getSelectedFile
            push hl
            ld de,FILE_INFO.attrib
            add hl,de
            ld a,(hl)
            and 010h
            pop hl
            jp z,ncDeleteRegular
            call buildSelectedFilePath
            jp c,deleteFileError
            ld a,CLI_OP_RMDIR
            jp ncDeleteReady
ncDeleteRegular:
            ld a,CLI_OP_DELETE
ncDeleteReady:
            MXOS_CALL MXOS_DOS_CLI
            jp c,       deleteFileError
            ; Количество файлов и курсор обновляются по реальному каталогу.
            jp          ncRefreshDirty

deleteFileError:
            MXOS_CALL MXOS_DOS_GET_ERROR
            call dosErrorText
            jp copyFileShowError
