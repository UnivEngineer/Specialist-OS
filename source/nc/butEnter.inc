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
            jp nz,  EnterCmdLine

            ; Получить имя выбранного файла
            call    getSelectedFile
            ld      a, (hl)
            inc     a
            jp z,   mainLoop ; Ничего не выбрано

            push    hl
            ld      de, FILE_INFO.attrib
            add     hl,de
            ld      a,(hl)
            and     010h
            pop     hl
            jp nz,  enterDirectory
            jp Enter_stdExec

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
            ; Команды CD/диск меняют общий контекст и активную панель.
            ld e,0
            call bios_fileGetSetDrive
            ld hl,state.panelA_drive
            call writeBytePanel
            call savePanelDirectory
            ld a,(ncCommandContext)
            or a
            jp nz,directoryChanged
            ld a,(ncCommandScreen)
            or a
            call nz,ncDamageBoth
            jp ncRefreshDirty
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
ncKeepFrames: DB 0
ncCommandErrorText: DB 'Command error',0
EnterExec:
            call ncStopObserver
            call saveState
            call ncFontRenderEnd
            ld hl,vars.cmdLine
            ; COM внутри BAT может затереть NC и вернуться обычным RET.
            ; Возвращаться следует в BIOS, который загрузит оболочку заново.
            ld de,bios_reboot
            push de
            jp bios_fileExec
