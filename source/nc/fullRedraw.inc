; Полное обновление обеих панелей: фон, рамки, метаданные, списки.
; Активная панель сохраняется; частичное обновление сюда не входит.
fullRedraw:
            ld a,1
            ld (ncInfoFreeDeferred),a
            ld a,(state.activePanel)
            push af
            COLOR COLOR_BORDER
fullClearPanels:
    IF NC_BANKED
            ld hl,09000h
            ld b,48
            ld c,230
            xor a
            call uiFillRectBytes
    ELSE
            ld hl,09000h
            ld b,48
fullClearColumn:
            ld l,0
            ld c,23
            xor a
fullClearPixels:
            DUP 10
            ld (hl),a
            inc l
            EDUP
            dec c
            jp nz,fullClearPixels
            inc h
            dec b
            jp nz,fullClearColumn
    ENDIF
fullDrawFrames:
            xor a
            ld (state.activePanel),a
            call drawFullPanelFrame
            ld a,1
            ld (state.activePanel),a
            call drawFullPanelFrame
            ; Сначала готовим метаданные активной панели.
            pop af
            push af
            ld (state.activePanel),a
fullDrawMetadata:
            call fullPanelMetadata
            ld a,(state.activePanel)
            xor 1
            ld (state.activePanel),a
            call fullPanelMetadata
fullDrawFiles:
            xor a
            ld (state.activePanel),a
            call fullPanelFiles
            ld a,1
            ld (state.activePanel),a
            call fullPanelFiles
            pop af
            ld (state.activePanel),a
            call fullPanelLoad
            ; Finish the slow number only after both panels are visible.
            xor a
            ld (ncInfoFreeDeferred),a
            ld a,(state.activePanel)
            push af
            xor 1
            ld (state.activePanel),a
            ld hl,state.panelA_info
            call readBytePanel
            or a
            call nz,pip_freeSpaceLast
            pop af
            ld (state.activePanel),a
            ret

; Рамка без заливки уже очищенной внутренней области.
drawFullPanelFrame:
            call ncResetPanelBorder
            call drawPanelHeader
            ld hl,g_panelEdges
            call draw
            ld hl,state.panelA_info
            call readBytePanel
            or a
            jp nz,fullInfoDividers
            call ncDrawFileDividers
            jp fullFrameDone
fullInfoDividers:
            ld hl,g_infoDividers
fullFrameDividers:
            call draw
fullFrameDone:
            xor a
            ld hl,vars.drawnLeftA
            call writeBytePanel
            ld hl,vars.drawnRightA
            jp writeBytePanel

fullPanelMetadata:
            ld hl,state.panelA_info
            call readBytePanel
            or a
            jp nz,printInfoContents
            call fullPanelLoad
            call updateCommandPath
            call printPanelPath
            jp printInfoLine
fullPanelFiles:
            ld hl,state.panelA_info
            call readBytePanel
            or a
            ret nz
            call fullPanelLoad
            COLOR COLOR_FILE
            jp printWholeFileList
fullPanelLoad:
            ld a,(ncStartupPrepared)
            or a
            jp nz,lf_Cached ; First paint only selects an already prepared view.
            ld hl,state.panelA_drive
            call readBytePanel
            jp loadFiles

; Keep the console banner visible while both catalogs, indices, selections and
; absolute paths are prepared. Also prepare the inactive F3 panel's file view.
ncPreparePanels:
            ld a,(state.activePanel)
            push af
            xor a
            ld (state.activePanel),a
            call fullPanelLoad
            call getPanelPath
            ld a,1
            ld (state.activePanel),a
            call fullPanelLoad
            call getPanelPath
            pop af
            ld (state.activePanel),a
            jp fullPanelLoad ; Restore the active directory and window pointer.

ncStartupCopyright:
            COLOR COLOR_CMDSCREEN
            ld hl,aCommanderBuild
            call bios_printStringReal
            ld c,' '
            call bios_printChar
            ld hl,aCopyright
            call bios_printStringReal
            ld c,10
    IF NC_BANKED
            call bios_printChar
            jp ncBankFlush
    ELSE
            jp bios_printChar
    ENDIF
ncStartupPrepared: DB 0

; Команды uiDraw: боковые штрихи и три строки нижней рамки.
g_panelEdges:
            DB 3,9,090h,218,050h
            DB 3,9,0A7h,218,00Ah
            DB 1,227,090h,22,05Fh,0FAh
            DB 3,228,090h,1,040h
            DB 3,228,0A7h,1,002h
            DB 1,229,090h,22,07Fh,0FEh
            DB 0
