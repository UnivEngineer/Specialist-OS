; Адаптер координат NC к общему оконному ABI DOS.
; HL = список объектов. AF/BC/DE/HL и флаги разрушаются.
draw:
            ld      a, (state.activePanel)
            or      a
            jp z,   drawPanelOffset
            ld      a, 24
drawPanelOffset:
            jp      uiDraw

; HL = экранный адрес, сместить на активную панель; AF сохраняется.
activePanelPos:
            push    af
            ld      a, (state.activePanel)
            or      a
            jp z,   activePanelOffset
            ld      a, 24
activePanelOffset:
            add     a, h
            ld      h, a
            pop     af
            ret

; Очищаем верх панели с Y=0. Углы и горизонтальные линии начинаются
; на Y=4 и Y=6; над ними нет вертикальных штрихов и остатков старого пути.
drawPanelTop:
drawPanelHeader:
    IF NC_BANKED
            ld hl,09000h
            call activePanelPos
            ld de,panelHeaderLeft
            ld bc,0109h
            call ncTileColumn
            inc h
            ld de,panelHeaderMiddle
            ld b,22
            call ncTileColumn
            ld a,h
            add a,22
            ld h,a
            ld de,panelHeaderRight
            ld b,1
            call ncTileColumn
    ELSE
            ld hl,09000h
            call activePanelPos
            ld b,24
drawPanelHeaderColumn:
            ld de,panelHeaderMiddle
            ld a,b
            cp 24
            jp nz,drawPanelHeaderLast
            ld de,panelHeaderLeft
            jp drawPanelHeaderCopy
drawPanelHeaderLast:
            cp 1
            jp nz,drawPanelHeaderCopy
            ld de,panelHeaderRight
drawPanelHeaderCopy:
            ld c,9
            ld l,0
drawPanelHeaderRow:
            ld a,(de)
            ld (hl),a
            inc de
            inc l
            dec c
            jp nz,drawPanelHeaderRow
            inc h
            dec b
            jp nz,drawPanelHeaderColumn
            ; Верх разделителя колонок входит в очищенную полосу заголовка.
    ENDIF
            ld hl,state.panelA_info
            call readBytePanel
            or a
            ret nz
            jp ncDrawFileDividers
panelHeaderLeft:   DB 0,0,0,0,07Fh,040h,05Fh,050h,050h
panelHeaderMiddle: DB 0,0,0,0,0FFh,0,0FFh,0,0
panelHeaderRight:  DB 0,0,0,0,0FEh,2,0FAh,00Ah,00Ah

; Repaint the frame and dividers without clearing the file list.
ncUpdateCatalogFrame:
            ld a,(state.activePanel)
            call ncCatalogPanelOverflow
            or a
            jp z,ncCatalogFrameReady
            cp 1
            ld a,1
            jp nz,ncCatalogFrameReady
            ld (ncCatalogWarnPending),a
ncCatalogFrameReady:
            ld b,a
            ld hl,vars.panelFrameA
            call readBytePanel
            cp b
            ret z
            ld a,b
            call writeBytePanel
            call ncSetPanelBorder
            ld hl,g_panelEdges
            call draw
            ; Side strokes replace whole bytes. Draw the divider ends after
            ; them so the horizontal line can merge with the inner strokes.
            jp drawPanelHeader

ncResetPanelBorder:
            xor a
            ld hl,vars.panelFrameA
            call writeBytePanel
            COLOR COLOR_BORDER
            ret
ncSetPanelBorder:
            ld hl,vars.panelFrameA
            call readBytePanel
            call ncBorderColor
    IF ENABLE_COLOR
            jp ncSetColor
    ELSE
            ret
    ENDIF
ncBorderColor:
            or a
            ld a,COLOR_BORDER
            ret z
            ld a,COLOR_TRUNCATED
            ret
