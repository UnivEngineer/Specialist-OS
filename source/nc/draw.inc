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
            ld hl,state.panelA_info
            call readBytePanel
            ld b,0
            or a
            jp nz,drawHeaderDividerReady
            ld b,080h
drawHeaderDividerReady:
            ld hl,09C07h
            call activePanelPos
            ld (hl),b
            inc l
            ld (hl),b
            ret
panelHeaderLeft:   DB 0,0,0,0,07Fh,040h,05Fh,050h,050h
panelHeaderMiddle: DB 0,0,0,0,0FFh,0,0FFh,0,0
panelHeaderRight:  DB 0,0,0,0,0FEh,2,0FAh,00Ah,00Ah
