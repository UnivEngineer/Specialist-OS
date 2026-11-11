; Копирование выполняет DOS через собственную пачку в банке DOS.
copyFileInt:
            ld      hl, state.panelA_drive
            call    readBytePanel
            ld      (vars.copyRequest.sourceDrive), a
            MXOS_CALL MXOS_DOS_FILE_SELECT_DRIVE
            jp c,   copyFileError
            ld      a,DIR_OP_GET
            MXOS_CALL MXOS_DOS_DIRECTORY
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
            MXOS_CALL MXOS_DOS_FILE_SELECT_DRIVE
            call    restorePanelDirectory
            ld      a,(state.activePanel)
            xor     1
            ld      (state.activePanel),a
            ld      hl, vars.input
            ld      de, vars.copyRequest.destination
            call    bios_fileNamePrepare
            jp c,   copyFileError
            ld      a,DIR_OP_RESOLVED
            MXOS_CALL MXOS_DOS_DIRECTORY
            ex      de,hl
            ld      (vars.copyRequest.destinationDirectory),hl
            ld      e, 0
            call    bios_fileGetSetDrive
            ld      (vars.copyRequest.destinationDrive), a
            ld      hl, 0
            ld      (vars.copyRequest.buffer), hl
            ld      (vars.copyRequest.bufferSize), hl
            ld      hl,ncOverwritePrompt
            ld      (vars.copyRequest.onExists),hl
            ld      hl, vars.copyRequest
copyFileOperation:
            ld a,CLI_OP_COPY
            MXOS_CALL MXOS_DOS_CLI
            ret nc
            cp      FILE_OP_EXISTS
            jp z,   copyFileExists
            jp      copyFileError

copyFileError:
            MXOS_CALL MXOS_DOS_GET_ERROR
copyFileErrorCode:
            call dosErrorText
            jp      copyFileShowError

copyFileExists:
            MXOS_CALL MXOS_DOS_GET_ERROR
            call dosErrorText

copyFileShowError:
            ld      de, 04480h          ; CENTER_LINE 20,80h
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
            ld      hl, 03890h          ; x=114, нижняя линия знака y=144
            ld      (bios_vars.cursorY), hl
            ld      hl, vars.input
            call    ncErrorCompact
            jp      printStringInv

; Поле ошибки вмещает 26 знаков. Абсолютные пути сокращаются по компонентам,
; для команды/относительного имени показываем многоточие и конец строки.
; HL=строка (ASCIIZ или окончание CR), HL=vars.displayPath; AF/BC/DE разрушены.
ncErrorCompact:
            ld de,vars.displayPath
            ld c,0
ncErrorCopy:
            ld a,(hl)
            cp 020h
            jp c,ncErrorCopied
            ld (de),a
            inc hl
            inc de
            inc c
            ld a,c
            cp MAX_PATH-1
            jp c,ncErrorCopy
ncErrorCopied:
            xor a
            ld (de),a
            ld hl,vars.displayPath
            ld a,c
            cp 27
            ret c
            ; Сохраняем полный суффикс пути, когда это абсолютное имя.
            ld a,(vars.input+1)
            cp ':'
            ld b,26
            ld hl,vars.input
            jp z,compactPath
            ld hl,vars.displayPath
            ld a,c
            sub 23
            ld e,a
            ld d,0
            add hl,de
            ld de,vars.displayPath
            ld a,'.'
            ld (de),a
            inc de
            ld (de),a
            inc de
            ld (de),a
            inc de
ncErrorTail:
            ld a,(hl)
            ld (de),a
            inc hl
            inc de
            or a
            jp nz,ncErrorTail
            ld hl,vars.displayPath
            ret
