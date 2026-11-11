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
            MXOS_CALL MXOS_DOS_DIRECTORY
            ret
savePanelDirectory:
            ld a,DIR_OP_GET
            MXOS_CALL MXOS_DOS_DIRECTORY
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
            MXOS_CALL MXOS_DOS_CLI
            jp c,directoryFailure
            call savePanelDirectory
directoryChanged:
            ld a,1
            ld (ncKeepFrames),a
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
            call getPanelPath
            ld de,vars.path
            jp nc,copyPanelPath
            push af
            xor a
            ld (de),a
            pop af
            ret
copyPanelPath:
            ld a,(hl)
            ld (de),a
            inc hl
            inc de
            or a
            jp nz,copyPanelPath
            ret

; Полный путь принадлежит панели; ключ — диск и кластер каталога.
; HL=ASCIIZ, CF/A=ошибка DOS. Остальные регистры разрушены.
getPanelPath:
            ld hl,vars.pathValidA
            call readBytePanel
            or a
            jp z,panelPathMiss
            ld hl,state.panelA_drive
            call readBytePanel
            ld b,a
            ld hl,vars.pathDriveA
            call readBytePanel
            cp b
            jp nz,panelPathMiss
            ld hl,state.panelA_dirLow
            call readBytePanel
            ld b,a
            ld hl,vars.pathLowA
            call readBytePanel
            cp b
            jp nz,panelPathMiss
            ld hl,state.panelA_dirHigh
            call readBytePanel
            ld b,a
            ld hl,vars.pathHighA
            call readBytePanel
            cp b
            jp nz,panelPathMiss
            call panelPathBase
            or a
            ret
panelPathMiss:
            call panelPathBase
            push hl
            ld a,DIR_OP_GETCWD
            MXOS_CALL MXOS_DOS_DIRECTORY
            pop hl
            ret c
            push hl
            ld hl,state.panelA_drive
            call readBytePanel
            ld hl,vars.pathDriveA
            call writeBytePanel
            ld hl,state.panelA_dirLow
            call readBytePanel
            ld hl,vars.pathLowA
            call writeBytePanel
            ld hl,state.panelA_dirHigh
            call readBytePanel
            ld hl,vars.pathHighA
            call writeBytePanel
            ld a,1
            ld hl,vars.pathValidA
            call writeBytePanel
            pop hl
            or a
            ret
panelPathBase:
            ld hl,vars.pathA
            ld a,(state.activePanel)
            or a
            ret z
            ld hl,vars.pathB
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
            call bios_printStringReal
            ld c,'>'
            call ncPrintChar
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
            call bios_printStringReal
            ld hl,(bios_vars.cursorY)
            ld (vars.cmdLinePos),hl
            ret

 ; Заголовок панели: полный путь либо сокращение по границам компонентов.
printPanelPath:
            call getPanelPath
            ret c
            ld b,28
            call compactPath
            push hl
            COLOR 0F1h
            ld hl,00408h
            call setCursorPosPanel
            pop hl
            jp bios_printStringReal

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
