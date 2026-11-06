; Контексты двух панелей: по два байта в прежнем 12-байтном NC_STATE.
; AF/BC/DE/HL разрушены, кроме явно сохранённых регистров.
restorePanelDirectory:
            ld hl,state.panelA_dirLow
            call readBytePanel
            ld e,a
            ld hl,state.panelA_dirHigh
            call readBytePanel
            ld d,a
            ld a,DIR_OP_SET
            MXCALL MX_DOS_DIRECTORY
            ret
savePanelDirectory:
            ld a,DIR_OP_GET
            MXCALL MX_DOS_DIRECTORY
            ret c
            ld a,e
            ld hl,state.panelA_dirLow
            call writeBytePanel
            ld a,d
            ld hl,state.panelA_dirHigh
            jp writeBytePanel
resetPanelDirectory:
            xor a
            ld hl,state.panelA_dirLow
            call writeBytePanel
            ld hl,state.panelA_dirHigh
            jp writeBytePanel
enterDirectory:
            ld de,vars.pathArg
            call compactName
            dec de
            dec de
            xor a
            ld (de),a
            ld hl,vars.pathArg
            ld a,CLI_OP_CHDIR
            MXCALL MX_DOS_CLI
            jp c,directoryFailure
            call savePanelDirectory
directoryChanged:
            call directoryResetSelection
            jp mainReloadActive
directoryChangedAll:
            call directoryResetSelection
            jp mainReload
directoryResetSelection:
            xor a
            ld hl,state.panelA_curFile
            call writeBytePanel
            ld hl,state.panelA_firstFile
            call writeBytePanel
            ld hl,vars.cmdLine
            ld (hl),0
            ld (vars.cmdLineEnd),hl
            call saveState
            ret
directoryFailure:
            ld hl,directoryError
            call bios_printString
            call bios_getch
            jp directoryChanged
directoryError: DB ' DIRECTORY ERROR',0
; Полное приглашение хранится в path; экран показывает хвост длинного пути.
; При длинной команде строка прокручивается, исходные байты не обрезаются.
updateCommandPath:
            ld hl,vars.path
            ld a,DIR_OP_GETCWD
            MXCALL MX_DOS_DIRECTORY
            ret nc
            ld hl,vars.path
            ld (hl),0
            ret
redrawCommandLine:
            ld hl,(vars.cmdLineEnd)
            ld (hl),0
commandPathReady:
            COLOR COLOR_CMDLINE
            ld hl,001F3h
            ld (bios_vars.cursorY),hl
            ld b,63
            call printSpaces
            ld hl,001F3h
            ld (bios_vars.cursorY),hl
            ld hl,vars.path
            ld b,0
commandPathLength:
            ld a,(hl)
            or a
            jp z,commandPathTail
            inc b
            inc hl
            jp commandPathLength
commandPathTail:
            ld hl,vars.path
            ld a,b
            cp 24
            jp c,commandPathPrint
            sub 24
            ld e,a
            ld d,0
            add hl,de
            ld b,24
commandPathPrint:
            push bc
            call bios_printString
            ld c,'>'
            call bios_printChar
            pop bc
            ld hl,(bios_vars.cursorY)
            ld (vars.cmdTextPos),hl
            ld a,61
            sub b
            ld (vars.cmdCapacity),a
; Перерисовываем только текст при сдвиге окна, без предварительного стирания.
; Вход: cmdLineEnd и завершающий ноль; выход: cmdLinePos, cmdOffset.
; Разрушает AF/BC/DE/HL и флаги.
redrawCommandText:
            ld hl,(vars.cmdTextPos)
            ld (bios_vars.cursorY),hl
            ld a,(vars.cmdCapacity)
            ld c,a
            ld hl,(vars.cmdLineEnd)
            ld a,l
            sub vars.cmdLine & 0FFh
            sub c
            jp nc,commandOffsetReady
            xor a
commandOffsetReady:
            ld (vars.cmdOffset),a
            ld hl,vars.cmdLine
            ld e,a
            ld d,0
            add hl,de
commandText:
            call bios_printString
            ld hl,(bios_vars.cursorY)
            ld (vars.cmdLinePos),hl
            ret

 ; Заголовок панели: полный путь либо сокращение по границам компонентов.
printPanelPath:
            ld hl,vars.path
            ld a,DIR_OP_GETCWD
            MXCALL MX_DOS_DIRECTORY
            ret c
            ld hl,vars.path
            ld b,29
            call compactPath
            push hl
            COLOR 0F1h
            ld hl,00308h
            call setCursorPosPanel
            pop hl
            jp bios_printString

; HL — абсолютный путь, B — ширина поля (не менее PATH_RESERVED_LENGTH+12 для FAT 8.3).
; Выход: HL — vars.displayPath, исходная строка не изменена.
; AF/BC/DE разрушены. Сохраняем диск и максимально длинный полный суффикс.
compactPath:
            push hl
            ld c,0
compactPathLength:
            ld a,(hl)
            or a
            jp z,compactPathMeasured
            inc hl
            inc c
            jp compactPathLength
compactPathMeasured:
            pop hl
            ld de,vars.displayPath
            ld a,b
            cp c
            jp nc,compactPathCopy
            sub PATH_RESERVED_LENGTH
            ld b,a
            ld a,(hl)
            ld (de),a
            inc de
            inc hl
            inc hl
            inc hl
            dec c
            dec c
            dec c
            push hl
            push bc
            ld hl,compactPathPrefix
            ld b,PATH_PREFIX_LENGTH
compactPathPrefixCopy:
            ld a,(hl)
            ld (de),a
            inc hl
            inc de
            dec b
            jp nz,compactPathPrefixCopy
            pop bc
            pop hl
compactPathBoundary:
            ld a,b
            cp c
            jp nc,compactPathCopy
compactPathSkip:
            ld a,(hl)
            inc hl
            dec c
            cp 05Ch
            jp z,compactPathBoundary
            cp '/'
            jp z,compactPathBoundary
            jp compactPathSkip
compactPathCopy:
            ld a,(hl)
            ld (de),a
            inc hl
            inc de
            or a
            jp nz,compactPathCopy
            ld hl,vars.displayPath
            ret
