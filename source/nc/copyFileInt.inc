; Копирование выполняет DOS. NC задаёт имена, диски и собственный буфер.
COPY_BUFFER_END     EQU APP_MEMORY_END
COPY_BUFFER         EQU (NC_END + 255) & 0FF00h
COPY_BUFFER_SECTORS EQU (COPY_BUFFER_END - COPY_BUFFER) / 256

copyFileInt:
            ld      hl, state.panelA_drive
            call    readBytePanel
            ld      (vars.copyRequest.sourceDrive), a
            call    bios_fileSelectDrive
            jp c,   copyFileError
            call    getSelectedFile
            ld      de, vars.copyRequest.source
            ld      c, FILE_NAME_LENGTH + 3
copyFileNameLoop:
            ld      a, (hl)
            ld      (de), a
            inc     hl
            inc     de
            dec     c
            jp nz,  copyFileNameLoop
            ld      hl, vars.input
            ld      de, vars.copyRequest.destination
            call    bios_fileNamePrepare
            ld      e, 0
            call    bios_fileGetSetDrive
            ld      (vars.copyRequest.destinationDrive), a
            ld      hl, COPY_BUFFER
            ld      (vars.copyRequest.buffer), hl
            ld      hl, COPY_BUFFER_END - COPY_BUFFER
            ld      (vars.copyRequest.bufferSize), hl
            ld      hl, vars.copyRequest
            call    bios_fileCopy
            ret nc
            cp      FILE_OP_EXISTS
            jp z,   copyFileExists
            jp      copyFileError

copyFileError:
            ld      hl, aCopyFailed
            jp      copyFileShowError

copyFileExists:
            ld      hl, aFileAlreadyExists

copyFileShowError:
            ld      de, 04280h          ; CENTER_LINE 20,80h
            call    showFileErrorWindow

copyFileWaitRelease:
            call    bios_keyScan
            inc     a
            jp nz,  copyFileWaitRelease

copyFileAnyKey:
            call    bios_keyScan
            inc     a
            jp z,   copyFileAnyKey
            jp      mainReload

; Общий баннер ошибок копирования/удаления и защиты файла.
; Вход HL=сообщение, DE=координаты заголовка BIOS; имя в vars.input.
; AF/BC/DE/HL и флаги разрушаются. Ожидание клавиши выполняет вызывающий код.
showFileErrorWindow:
            push    hl
            push    de
            call    drawWindow2
            COLOR   COLOR_DIALOG_ERR
            pop     hl
            ld      (bios_vars.cursorY), hl
            pop     hl
            call    bios_printString
            COLOR   COLOR_DIALOG
            ld      hl, 05090h
            ld      (bios_vars.cursorY), hl
            ld      hl, vars.input
            jp      printStringInv
