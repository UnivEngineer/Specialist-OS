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
            call bios_fileSelectDrive
            call restorePanelDirectory
            ld hl,vars.cmdLine
            ld a,CLI_OP_QUERY
            call bios_cli
            jp c,ncCommandError
            ld a,b
            and 1
            ld (ncCommandScreen),a
            jp z,ncCommandRun
            COLOR COLOR_CMDSCREEN
            ld c,01Fh
            call bios_printChar
ncCommandRun:
            ld hl,vars.cmdLine
            ld de,ncOverwritePrompt
            ld a,CLI_OP_COMMAND
            call bios_cli
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
            call z,drawToolbar
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
            call saveState
            jp ncRefreshDirty
ncCommandError:
            call bios_getError
            call dosErrorText
            jp copyFileShowError
ncCommandScreen: DB 0
ncCommandContext: DB 0
ncKeepFrames: DB 0
ncCommandErrorText: DB 'Command error',0
EnterExec:
            call ncStopObserver
            call saveState
            ld hl,vars.cmdLine
            jp bios_fileExec
