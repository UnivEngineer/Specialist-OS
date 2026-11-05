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
            ld      a,DIR_OP_GET
            call    bios_directory
            ex      de,hl
            ld      (vars.copyRequest.sourceDirectory),hl
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
            ; По умолчанию назначение — каталог противоположной панели.
            ld      a,(state.activePanel)
            xor     1
            ld      (state.activePanel),a
            ld      hl,state.panelA_drive
            call    readBytePanel
            call    bios_fileSelectDrive
            call    restorePanelDirectory
            ld      a,(state.activePanel)
            xor     1
            ld      (state.activePanel),a
            ld      hl, vars.input
            ld      de, vars.copyRequest.destination
            call    bios_fileNamePrepare
            jp c,   copyFileError
            ld      a,DIR_OP_RESOLVED
            call    bios_directory
            ex      de,hl
            ld      (vars.copyRequest.destinationDirectory),hl
            ld      e, 0
            call    bios_fileGetSetDrive
            ld      (vars.copyRequest.destinationDrive), a
            ld      hl, COPY_BUFFER
            ld      (vars.copyRequest.buffer), hl
            call    getCopyBufferSize
            jp c,   copyFileError
            ld      (vars.copyRequest.bufferSize), hl
            ld      hl,ncOverwritePrompt
            ld      (vars.copyRequest.onExists),hl
            ld      hl, vars.copyRequest
copyFileOperation:
            ld a,CLI_OP_COPY
            call    bios_cli
            ret nc
            cp      FILE_OP_EXISTS
            jp z,   copyFileExists
            jp      copyFileError

; HL=доступные целые секторы буфера; учитывает текущий системный memtop.
; AF/DE/HL разрушены, BC сохранён; CF=нет хотя бы одного сектора.
getCopyBufferSize:
            call bios_getMemTop
            ld de,COPY_BUFFER_END-1
            call bios_cmp_hl_de
            jp c,getCopyLimitReady
            ex de,hl
getCopyLimitReady:
            inc hl
            ld de,COPY_BUFFER
            call bios_sub_hl_de
            ret c
            ld l,0
            ld a,h
            or a
            ret nz
            scf
            ret

copyFileError:
            call bios_getError
copyFileErrorCode:
            call dosErrorText
            jp      copyFileShowError

copyFileExists:
            call bios_getError
            call dosErrorText

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
            jp      ncRefreshDirty

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
