; Контексты двух панелей: по два байта в прежнем 12-байтном NC_STATE.
; AF/BC/DE/HL разрушены, кроме явно сохранённых регистров.
restoreActiveDirectory:
            ld hl,state.panelA_drive
            call readBytePanel
            MXOS_CALL MXOS_DOS_FILE_SELECT_DRIVE
            ret c
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
            call restoreActiveDirectory
            jp c,directoryFailure
            ld hl,vars.pathArg
            ld a,CLI_OP_CHDIR
            MXOS_CALL MXOS_DOS_CLI
            jp c,directoryFailure
            call savePanelDirectory
directoryChanged:
            call ncParentPrepare
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
            push af
            ld hl,vars.pathArg
            ld de,vars.input
directoryErrorPath:
            ld a,(hl)
            ld (de),a
            inc hl
            inc de
            or a
            jp nz,directoryErrorPath
            pop af
            jp copyFileErrorCode

; The old prompt path still identifies the directory we just left. Only an
; immediate parent transition selects its leaf; entering children / other drives
; keeps the usual initial selection. No saved ordinal can survive re-sorting.
ncParentPrepare:
            xor a
            ld (ncParentPending),a
            call getPanelPath
            ret c
            ld de,vars.path
ncParentPrefix:
            ld a,(hl)
            or a
            jp z,ncParentPrefixEnd
            call bios_upperCase
            ld b,a
            ld a,(de)
            call bios_upperCase
            cp b
            ret nz
            inc hl
            inc de
            jp ncParentPrefix
ncParentPrefixEnd:
            dec hl
            ld a,(hl)
            cp 05Ch ; Root already includes its separator.
            jp z,ncParentLeaf
            ld a,(de)
            cp 05Ch
            ret nz
            inc de
ncParentLeaf:
            ld hl,ncParentName
            ld b,13 ; Maximum 8.3 leaf plus NUL.
ncParentCopy:
            ld a,(de)
            cp 05Ch ; CD ../.. is not an immediate parent transition.
            ret z
            call bios_upperCase
            ld (hl),a
            inc hl
            inc de
            or a
            jp z,ncParentCopied
            dec b
            jp nz,ncParentCopy
            ret
ncParentCopied:
            ld a,(ncParentName)
            or a
            ret z
            ld a,1
            ld (ncParentPending),a
            ret

; Called once after acquiring the parent's current index. Find the directory,
; not a similarly named file, then let loadFiles clamp/adjust the visible window.
ncParentSelect:
            ld a,(ncParentPending)
            or a
            ret z
            xor a
            ld (ncParentPending),a
            ld hl,ncParentName
            ld a,10h
; HL=uppercase leaf, A=required attributes (0 means any file/directory).
ncSelectLeaf:
            ld (ncSelectName),hl
            ld (ncSelectAttrib),a
            call fileIndexBase
            ld c,0
ncParentFind:
            ld a,(hl)
            cp 0FFh
            ret z
            push hl
            call indexedFile
            push hl
            ld de,FILE_INFO.attrib
            add hl,de
            ld a,(ncSelectAttrib)
            or a
            jp z,ncSelectAny
            and (hl)
            pop hl
            jp z,ncParentNext
            jp ncSelectCompareName
ncSelectAny:
            pop hl
ncSelectCompareName:
            ld de,vars.pathArg
            call compactName
            ld hl,vars.pathArg
            push hl
            ld hl,(ncSelectName)
            ex de,hl
            pop hl
ncParentCompare:
            ld a,(hl)
            call bios_upperCase
            ld b,a
            ld a,(de)
            cp b
            jp nz,ncParentNext
            inc hl
            inc de
            or a
            jp nz,ncParentCompare
            pop hl
            ld a,c
            ld hl,state.panelA_curFile
            jp writeBytePanel
ncParentNext:
            pop hl
            inc hl
            inc c
            jp ncParentFind
ncParentPending:    DB     0
ncParentName:       BLOCK  13,0
ncSelectName:       DW     0
ncSelectAttrib:     DB     0

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
            ld hl,vars.panelA_diskError
            call readBytePanel
            cp DOS_ERR_NO_MEDIA
            jp z,panelNoMediaPath
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
            jp nc,panelPathStore
            cp DOS_ERR_NO_MEDIA
            scf
            ret nz
panelNoMediaPath:
            call panelPathBase
            ; Без тома заголовок и приглашение по-прежнему обозначают диск.
            push hl
            ld hl,state.panelA_drive
            call readBytePanel
            pop hl
            push hl
            add a,'A'
            ld (hl),a
            inc hl
            ld (hl),':'
            inc hl
            ld (hl),05Ch
            inc hl
            ld (hl),0
            pop hl
            or a
            ret
panelPathStore:
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
            ld hl,000F3h
            ld (bios_vars.cursorY),hl
            ld hl,vars.path
            ld b,24
            call compactPath
            push hl
            ld b,0
commandPathLength:
            ld a,(hl)
            or a
            jp z,commandPathTail
            inc b
            inc hl
            jp commandPathLength
commandPathTail:
            ld (hl),'>'
            inc hl
            ld (hl),0
            pop hl
commandPathPrint:
            push bc
            call bios_printStringReal
            pop bc
            ld hl,(bios_vars.cursorY)
            ld (vars.cmdTextPos),hl
            ld a,(ncCommandChars)
            dec a
            sub b
            ld (vars.cmdCapacity),a
; Перерисовываем только текст при сдвиге окна, без предварительного стирания.
; Вход: cmdLineEnd и завершающий ноль; выход: cmdLinePos, cmdOffset.
; Разрушает AF/BC/DE/HL и флаги.
redrawCommandText:
            COLOR COLOR_CMDLINE ; This entry also follows a colored file cursor.
            ld hl,(vars.cmdTextPos)
            ld (bios_vars.cursorY),hl
            ld a,(vars.cmdCapacity)
            ld c,a
            ld hl,(vars.cmdLineEnd)
            ld a,l
            sub vars.cmdLine & 0FFh
            sub c
            jp c,commandOffsetZero
            inc a ; Keep a cell for the editor cursor at the right edge.
            jp commandOffsetReady
commandOffsetZero:
            xor a
commandOffsetReady:
            ld (vars.cmdOffset),a
            ld hl,vars.cmdLine
            ld e,a
            ld d,0
            add hl,de
commandText:
            ld a,(vars.cmdCapacity)
            ld b,a
            call ncPrintField ; Erase the old tail in the same pass as new text.
            ld a,(vars.cmdLineEnd)
            sub vars.cmdLine & 0FFh
            ld b,a
            ld a,(vars.cmdOffset)
            ld c,a
            ld a,b
            sub c
            call ncTextAdvance
            ld hl,(vars.cmdTextPos)
            add a,h
            ld h,a
            ld (bios_vars.cursorY),hl
            ld (vars.cmdLinePos),hl
            ret

 ; Заголовок панели: полный путь либо сокращение по границам компонентов.
printPanelPath:
            call ncUpdateCatalogFrame
            call getPanelPath
            ret c
            ld a,(ncHeaderChars)
            ld b,a
            call compactPath
            push hl
            call ncSetPanelBorder
            ld hl,00408h
            call setCursorPosPanel
            pop hl
            jp bios_printStringReal

 ; HL=absolute ASCIIZ path, B=available characters (>=19 for full FAT 8.3).
; Preserve complete components, a useful leading directory when it fits, and
; the longest complete suffix. Only the display copy is shortened.
compactPath:
            ld (ncPathSource),hl
            ld a,b
            ld (ncPathWidth),a
            ld c,0
            ld b,0
ncPathMeasure:
            ld a,(hl)
            or a
            jp z,ncPathMeasured
            inc hl
            inc c
            inc b
            cp 05Ch
            jp z,ncPathSeparator
            cp '/'
            jp nz,ncPathMeasure
ncPathSeparator:
            ld b,0             ; length of the final component
            jp ncPathMeasure
ncPathMeasured:
            ld a,c
            ld (ncPathLength),a
            ld a,(ncPathWidth)
            cp c
            ld hl,(ncPathSource)
            ld de,vars.displayPath
            jp nc,compactPathCopy
            ; Default prefix is the drive and root separator.
            ld a,3
            ld (ncPathHead),a
            push bc
            ld de,3
            add hl,de
            ld c,3
ncPathFirstComponent:
            ld a,(hl)
            or a
            jp z,ncPathPrefixDone
            inc hl
            inc c
            cp 05Ch
            jp z,ncPathTryPrefix
            cp '/'
            jp nz,ncPathFirstComponent
ncPathTryPrefix:
            ; Prefix + omission mark + final component must fit.
            ld a,(ncPathWidth)
            sub c
            jp c,ncPathPrefixDone
            sub PATH_OMISSION_LENGTH+1
            jp c,ncPathPrefixDone
            cp b
            jp c,ncPathPrefixDone
            ld a,c
            ld (ncPathHead),a
ncPathPrefixDone:
            pop bc
            ld a,(ncPathHead)
            ld c,a
            ld b,a
            ld hl,(ncPathSource)
            ld de,vars.displayPath
ncPathCopyHead:
            ld a,(hl)
            ld (de),a
            inc hl
            inc de
            dec b
            jp nz,ncPathCopyHead
            push hl
            push bc
            ld hl,pathOmissionMark
            ld b,PATH_OMISSION_LENGTH+1
ncPathCopyMark:
            ld a,(hl)
            ld (de),a
            inc hl
            inc de
            dec b
            jp nz,ncPathCopyMark
            pop bc
            pop hl
            ld a,(ncPathWidth)
            sub c
            sub PATH_OMISSION_LENGTH+1
            ld b,a            ; suffix budget
            ld a,(ncPathLength)
            sub c
            ld c,a            ; remaining input
ncPathFindSuffix:
            ld a,b
            cp c
            jp nc,compactPathCopy
ncPathSkipComponent:
            ld a,(hl)
            or a
            jp z,compactPathCopy ; malformed/overlong input cannot run past NUL
            inc hl
            dec c
            cp 05Ch
            jp z,ncPathFindSuffix
            cp '/'
            jp z,ncPathFindSuffix
            jp ncPathSkipComponent
compactPathCopy:
            ld a,(hl)
            ld (de),a
            inc hl
            inc de
            or a
            jp nz,compactPathCopy
            ld hl,vars.displayPath
            ret
ncPathSource:   DW 0
ncPathWidth:    DB 0
ncPathLength:   DB 0
ncPathHead:     DB 0
