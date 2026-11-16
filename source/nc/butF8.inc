;----------------------------------------------------------------------------
; MXOS NC.COM
; Клавиша F8 - удаление файла
;
; 2013-12-18 Дизассемблировано vinxru
;----------------------------------------------------------------------------

but_F8:     ; Рисуем окно
            call    drawWindowIfSel

            ; Выводим "DELETE FROM A:"
            ld hl,state.panelA_drive
            call readBytePanel
            add a,'A'
            ld (aDeleteFrom+12),a
            ld hl,aDeleteFrom
            ld e,94
            call ncPrintCentered

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
            call restorePanelDirectory
            ld a,(state.activePanel)
            ld hl,state.panelA_drive
            call readBytePanel
            ld (vars.copyRequest.sourceDrive),a
            ld a,DIR_OP_GET
            MXOS_CALL MXOS_DOS_DIRECTORY
            ex de,hl
            ld (vars.copyRequest.sourceDirectory),hl
            call getSelectedFile
            ld de,vars.copyRequest.source
            ld b,11
ncDeleteName:
            ld a,(hl)
            ld (de),a
            inc hl
            inc de
            dec b
            jp nz,ncDeleteName
            ld a,CLI_OP_DELETE
            call ncFileops
            jp c,deleteFileJobError
            ; Количество файлов и курсор обновляются по реальному каталогу.
            jp          ncRefreshDirty

deleteFileError:
            MXOS_CALL MXOS_DOS_GET_ERROR
deleteFileJobError:
            call dosErrorText
            jp copyFileShowError
