;----------------------------------------------------------------------------
; MXOS NC.COM
; Клавиша ENTER
;
; 2013-12-18 Дизассемблировано vinxru
;----------------------------------------------------------------------------

but_Enter:  ; Если ком строка не пустая
            ld      hl, (vars.cmdLineEnd)
            ld      a, vars.cmdLine & 0FFh
            cp      l
            jp nz,  ncEnterTypedCommand

            ; Получить имя выбранного файла
            call    getSelectedFile
            ld      a, (hl)
            inc     a
            jp z,   mainLoop_cursorReady ; Ничего не выбрано

            push    hl
            ld      de, FILE_INFO.attrib
            add     hl,de
            ld      a,(hl)
            and     010h
            pop     hl
            jp nz,  ncEnterDirectory
            ; Check before building a command or touching the console screen.
            call ncEnterFileAction
            jp c,mainLoop_cursorReady
            call ncEnterHideCursor
            jp Enter_stdExec

ncEnterTypedCommand:
            ; Quiet commands may not touch either visible directory. Keep the
            ; highlight until their change notifications say a redraw is needed.
            ld a,1
            ld (ncCommandCursorKept),a
            jp EnterCmdLine
ncEnterDirectory:
            push hl
            call ncEnterHideCursor
            pop hl
            jp enterDirectory
ncEnterHideCursor:
            COLOR COLOR_FILE
            jp drawCursor

; HL=FILE_INFO. CF=0: native executable type or registered association.
; Other extensions leave the command line and both panels untouched.
ncEnterFileAction:
            ld de,FILE_INFO.ext
            add hl,de
            push hl
            ld a,(hl)
            call bios_upperCase
            ld b,a
            inc hl
            ld a,(hl)
            call bios_upperCase
            ld c,a
            inc hl
            ld a,(hl)
            call bios_upperCase
            ld d,a
            ld a,b
            cp 'C'
            jp z,ncEnterCom
            cp 'E'
            jp z,ncEnterExe
            cp 'B'
            jp nz,ncEnterAssociation
            ld a,c
            cp 'A'
            jp nz,ncEnterAssociation
            ld a,d
            cp 'T'
            jp z,ncEnterNative
            jp ncEnterAssociation
ncEnterCom:
            ld a,c
            cp 'O'
            jp nz,ncEnterAssociation
            ld a,d
            cp 'M'
            jp z,ncEnterNative
            jp ncEnterAssociation
ncEnterExe:
            ld a,c
            cp 'X'
            jp nz,ncEnterAssociation
            ld a,d
            cp 'E'
            jp z,ncEnterNative
ncEnterAssociation:
            pop hl
            ; Query uses the CLI-owned association buffer inside banked DOS.
            ; Sort keys are transient; no index or FILE_INFO is overwritten.
            ld de,NC_SORT_KEYS
            ld b,3
ncEnterCopyExtension:
            ld a,(hl)
            ld (de),a
            inc hl
            inc de
            dec b
            jp nz,ncEnterCopyExtension
            xor a
            ld (de),a
            ld hl,NC_SORT_KEYS
            ld a,CLI_OP_ASSOCIATION
            MXOS_CALL MXOS_DOS_CLI
            ret
ncEnterNative:
            pop hl
            or a
            ret

Enter_5:   ; F4 задаёт явную команду редактора, ассоциации остаются в CLI.
            ld hl,vars.cmdLine
EnterEditorCopy:
            ld a,(de)
            cp 0Dh
            jp z,EnterEditorArgument
            ld (hl),a
            inc hl
            inc de
            jp EnterEditorCopy
EnterEditorArgument:
            ld (hl),' '
            inc hl
            ld (hl),0
            ld (vars.cmdLineEnd),hl
Enter_stdExec:
            ; Получить выбранный файл
            call     getSelectedFile

            ; Ничего не выбрано
            ld      a, (hl)
            inc     a
            jp z,   mainLoop

            ; ?
            call    selFileToCmdLine
            jp c,   mainReload

EnterCmdLine:
            ld hl,(vars.cmdLineEnd)
            ld (hl),0
            ld hl,state.panelA_drive
            call readBytePanel
            MXOS_CALL MXOS_DOS_FILE_SELECT_DRIVE
            call restorePanelDirectory
            ld hl,vars.cmdLine
            ld a,CLI_OP_QUERY
            MXOS_CALL MXOS_DOS_CLI
            jp c,ncCommandError
            ld a,b
            and 1
            ld (ncCommandScreen),a
            jp z,ncCommandRun
            xor a
            ld (ncCommandCursorKept),a
            COLOR COLOR_CMDSCREEN
            ld c,01Fh
            call bios_printChar
            ld hl,8
            call bios_setCursorPos
            ld hl,vars.cmdLine
            call bios_printString
            ld c,0Ah
            call bios_printChar
ncCommandRun:
            ld hl,vars.cmdLine
            ld de,ncOverwritePrompt
            ld a,CLI_OP_COMMAND
            MXOS_CALL MXOS_DOS_CLI
            jp c,ncCommandError
            cp 0FFh
            jp z,EnterExec
            ld a,b
            and 4
            ld (ncCommandContext),a
            ld a,b
            and 1
            call nz,bios_getch
            ld a,(ncCommandScreen)
            xor 1
            ld (ncKeepFrames),a
            or a
            call z,ncCommandRestoreScreen
            ; CLI's context flag describes the command, not whether it moved.
            ; Preserve the list/cursor when the actual drive+directory is equal.
            call ncCommandUpdateContext
ncCommandContextReady:
            ld a,(ncCommandContext)
            or a
            jp z,ncCommandSameContext
            call ncCommandHideCursor
            jp directoryChanged
ncCommandSameContext:
            ld a,(ncCommandScreen)
            or a
            call nz,ncDamageBoth
            jp ncRefreshDirty

; Compare the returned DOS context with the panel BEFORE updating its state.
; ncCommandContext=0/4 means unchanged/changed. Failed GET keeps CLI's flag.
ncCommandUpdateContext:
            ld e,0
            call bios_fileGetSetDrive
            ld b,a
            ld hl,state.panelA_drive
            call readBytePanel
            cp b
            ld a,0
            jp z,ncCommandDriveCompared
            ld a,4
ncCommandDriveCompared:
            push af
            ld a,b
            call writeBytePanel
            ld a,DIR_OP_GET
            MXOS_CALL MXOS_DOS_DIRECTORY
            jp c,ncCommandContextUnknown
            pop bc ; B=drive change flag; DE=new directory cluster.
            ld hl,state.panelA_dirLow
            call readBytePanel
            cp e
            jp nz,ncCommandDirectoryChanged
            ld hl,state.panelA_dirHigh
            call readBytePanel
            cp d
            jp nz,ncCommandDirectoryChanged
            ld a,b
            jp ncCommandContextStore
ncCommandDirectoryChanged:
            ld a,4
ncCommandContextStore:
            ld (ncCommandContext),a
            ld a,e
            ld hl,state.panelA_dirLow
            call writeBytePanel
            ld a,d
            ld hl,state.panelA_dirHigh
            jp writeBytePanel
ncCommandContextUnknown:
            pop af
            ret
; Консольный вывод затрагивает и промежутки вне панелей/строки ввода.
; Полная очистка нужна только при возврате с консольного экрана.
ncCommandRestoreScreen:
            COLOR COLOR_HELP_F
            ld c,01Fh
            call bios_printChar
            jp drawToolbar
ncCommandError:
            MXOS_CALL MXOS_DOS_GET_ERROR
            call dosErrorText
            push hl
            ; В командном режиме поле диалога содержит ошибочную команду.
            ld hl,vars.cmdLine
            ld de,vars.input
ncCommandErrorCopy:
            ld a,(hl)
            ld (de),a
            inc hl
            inc de
            or a
            jp nz,ncCommandErrorCopy
            pop hl
            jp copyFileShowError
ncCommandScreen: DB 0
ncCommandContext: DB 0
ncCommandCursorKept: DB 0
; Only a retained command highlight is erased; callers own the next redraw.
ncCommandHideCursor:
            ld a,(ncCommandCursorKept)
            or a
            ret z
            xor a
            ld (ncCommandCursorKept),a
            jp ncEnterHideCursor
ncKeepFrames: DB 0
ncCommandErrorText: DB 'Command error',0
EnterExec:
            call ncStopObserver
            call saveState
            ld hl,vars.cmdLine
            ; COM внутри BAT может затереть NC и вернуться обычным RET.
            ; Возвращаться следует в BIOS, который загрузит оболочку заново.
            ld de,bios_reboot
            push de
            jp bios_fileExec
