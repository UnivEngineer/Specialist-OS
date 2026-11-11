; Полное обновление обеих панелей: фон, рамки, метаданные, списки.
; Активная панель сохраняется; частичное обновление сюда не входит.
fullRedraw:
            ld a,(state.activePanel)
            push af
            COLOR COLOR_BORDER
fullClearPanels:
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
fullDrawFrames:
            xor a
            ld (state.activePanel),a
            call drawFullPanelFrame
            ld a,1
            ld (state.activePanel),a
            call drawFullPanelFrame
            ; Активная файловая панель готовится первой: информационная
            ; панель использует её счётчик файлов.
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
            jp fullPanelLoad

; Рамка без заливки уже очищенной внутренней области.
drawFullPanelFrame:
            COLOR COLOR_BORDER
            call drawPanelHeader
            ld hl,g_panelEdges
            call draw
            ld hl,state.panelA_info
            call readBytePanel
            or a
            ld hl,g_fileDividers
            jp z,fullFrameDividers
            ld hl,g_infoDividers
fullFrameDividers:
            call draw
            xor a
            ld hl,vars.drawnLeftA
            call writeBytePanel
            ld hl,vars.drawnRightA
            jp writeBytePanel

fullPanelMetadata:
            call fullPanelLoad
            ld hl,state.panelA_info
            call readBytePanel
            or a
            jp nz,printInfoContents
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
            ld hl,state.panelA_drive
            call readBytePanel
            jp loadFiles

; Команды uiDraw: боковые штрихи и три строки нижней рамки.
g_panelEdges:
            DB 3,9,090h,218,050h
            DB 3,9,0A7h,218,00Ah
            DB 1,227,090h,22,05Fh,0FAh
            DB 3,228,090h,1,040h
            DB 3,228,0A7h,1,002h
            DB 1,229,090h,22,07Fh,0FEh
            DB 0
