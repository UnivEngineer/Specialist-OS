; Копирование выполняет DOS через собственную пачку в банке DOS.
copyFileInt:
            ld      hl, state.panelA_drive
            call    readBytePanel
            ld      (vars.copyRequest.sourceDrive), a
            MXOS_CALL_REGS MXOS_DOS_FILE_SELECT_DRIVE
            call nc,restorePanelDirectory
            jp c,   copyFileError
            ld      a,DIR_OP_GET
            MXOS_CALL_REGS MXOS_DOS_DIRECTORY
            ex      de,hl
            ld      (vars.copyRequest.sourceDirectory),hl
            call    getSelectedFile
            ld      de, vars.copyRequest.source
            call    ncCopyShortName
            ; По умолчанию назначение — каталог противоположной панели.
            ld      a,(state.activePanel)
            xor     1
            ld      (state.activePanel),a
            ld      hl,state.panelA_drive
            call    readBytePanel
            MXOS_CALL_REGS MXOS_DOS_FILE_SELECT_DRIVE
            call    restorePanelDirectory
            ld      a,(state.activePanel)
            xor     1
            ld      (state.activePanel),a
            ld      hl, vars.input
            ld      de, vars.copyRequest.destination
            MXOS_CALL_REGS MXOS_DOS_FILE_NAME_PREPARE
            jp c,   copyFileError
            ld      a,(vars.copyRequest.destination)
            cp      ' '
            jp nz,  ncCopyDestinationReady
            ; A drive or a path ending in a separator supplies the directory
            ; only. DOS has already resolved it; keep the source leaf name.
            ld      a,(vars.input)
            or      a
            jp z,   ncCopyBadDestination
            ld      a,(hl)
            or      a
            jp nz,  ncCopyBadDestination
            dec     hl
            ld      a,(hl)
            cp      ':'
            jp z,   ncCopyUseSourceName
            cp      '/'
            jp z,   ncCopyUseSourceName
            cp      05Ch
            jp nz,  ncCopyBadDestination
ncCopyUseSourceName:
            ld      hl,vars.copyRequest.source
            ld      de,vars.copyRequest.destination
            call    ncCopyShortName
ncCopyDestinationReady:
            ld      a,DIR_OP_RESOLVED
            MXOS_CALL_REGS MXOS_DOS_DIRECTORY
            ex      de,hl
            ld      (vars.copyRequest.destinationDirectory),hl
            ld      e, 0
            MXOS_CALL_REGS MXOS_DOS_FILE_GET_SET_DRIVE
            ld      (vars.copyRequest.destinationDrive), a
            ld      hl, 0
            ld      (vars.copyRequest.buffer), hl
            ld      (vars.copyRequest.bufferSize), hl
            ld      hl,ncOverwritePrompt
            ld      (vars.copyRequest.onExists),hl
            ld      hl, vars.copyRequest
copyFileOperation:
            ld a,CLI_OP_COPY
            call ncFileops
            ret nc
            cp      FILE_OP_EXISTS
            jp z,   copyFileExists
            jp      copyFileErrorCode

ncCopyBadDestination:
            ld      a,DOS_ERR_ARGUMENT
            jp      copyFileErrorCode

ncCopyShortName:
            ld      c,FILE_NAME_LENGTH + 3
ncCopyShortNameByte:
            ld      a,(hl)
            ld      (de),a
            inc     hl
            inc     de
            dec     c
            jp nz,  ncCopyShortNameByte
            ret

copyFileError:
            MXOS_CALL_REGS MXOS_DOS_GET_ERROR
copyFileErrorCode:
            call dosErrorText
            jp      copyFileShowError

copyFileExists:
            MXOS_CALL_REGS MXOS_DOS_GET_ERROR
            call dosErrorText

copyFileShowError:
            call    showFileErrorWindow

    IF NC_BANKED
            call ncBankFlush
    ENDIF
copyFileWaitRelease:
            call    bios_keyScan
            inc     a
            jp nz,  copyFileWaitRelease

copyFileAnyKey:
            call uiModalGetch
            cp 13
            jp z,ncRefreshDirty
            cp 01Bh
            jp z,ncRefreshDirty
            jp copyFileAnyKey

; Общий баннер ошибок копирования/удаления и защиты файла.
; Вход HL=сообщение, DE=координаты заголовка BIOS; имя в vars.input.
; AF/BC/DE/HL и флаги разрушаются. Ожидание клавиши выполняет вызывающий код.
showFileErrorWindow:
            push hl
            ld hl,ncErrorModal
            call ncModalOpen
            pop hl
            call uiModalTitle
            ld hl,240
            call ncProgressFieldWidth
            ld a,b
            ld (ncErrorChars),a
            ld hl,vars.input
            call ncErrorCompact
            jp uiModalBodyText
ncErrorHint: DB 'OK [Enter]',0
ncErrorModal:
    UI_MODAL_BEGIN 12,1,1
ncOkButton:
    UI_MODAL_BUTTON ncErrorHint,0,0,1
    UI_MODAL_END

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
            ld b,a
            ld a,(ncErrorChars)
            cp b
            ld a,b
            ret nc
            ; Сохраняем полный суффикс пути, когда это абсолютное имя.
            ld a,(vars.input+1)
            cp ':'
            push af
            ld a,(ncErrorChars)
            ld b,a
            pop af
            ld hl,vars.input
            jp z,compactPath
            ld hl,vars.displayPath
            ld a,c
            push bc
            ld b,a
            ld a,(ncErrorChars)
            sub 3
            ld c,a
            ld a,b
            sub c
            pop bc
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
