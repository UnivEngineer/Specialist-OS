;----------------------------------------------------------------------------
; MXOS NC.COM
; Рисование курсора
;
; 2013-12-18 Дизассемблировано vinxru
;----------------------------------------------------------------------------

drawCursor: ; Цвет устаналвиается перед выводом

            ; Если файлов нет, выходим
            ld    hl, state.panelA_filesCnt
            call  readBytePanel
            and   a
    IF ENABLE_COLOR
            ret Z 
    ELSE
            jp z, drawCursor_0
    ENDIF
            ; C какого файла начинается список файлов на панели
            ld    hl, state.panelA_firstFile
            call  readBytePanel
            ld    c, a

            ; Номер выбранного файла
            ld    hl, state.panelA_curFile
            call  readBytePanel
            sub   c

            call ncSlotPosition
            ld a,d
            rrca
            rrca
            add a,90h
            ld h,a
            ld a,e
            sub 8
            ld l,a
            ld a,(ncCellBytes)
            ld b,a
            ld a,(ncFontWidth)
            cp 6
            jp nz,drawCursorWidth
            dec b ; Twelve 6px characters: 72 pixels, then a 1px border.
drawCursorWidth:
            ld c,10

            ; Если правая панель, сдвигаем курсор на пол экрана
drawCursor_2:
            call  activePanelPos
    IF NC_FILE_COLORS
            push hl
            push bc
            push de
            ld a,(ncCurrentColor)
            cp COLOR_CURSOR
            jp z,drawCursorColorReady
            call ncRowColorAt
            ld a,(hl)
            or a
            jp nz,drawCursorNormalColor
            ld a,COLOR_FILE ; Empty/unpainted fixture or newly opened panel.
drawCursorNormalColor:
            call ncSetColor
drawCursorColorReady:
            pop de
            pop bc
            pop hl
    ENDIF
    IF ENABLE_COLOR
            jp uiRecolorRectBytes
    ELSE
            call  inverseRect
            ; Only 6px has a spare byte with no frame pixels. Keep 4/8px
            ; selection byte-aligned rather than recoloring the panel frame.
            ld a,(ncFontWidth)
            cp 6
            ret nz
            ld a,d
            ld h,0
            ld l,a
            ld a,(state.activePanel)
            or a
            jp z,drawCursorPixelX
            ld bc,96
            add hl,bc
drawCursorPixelX:
            add hl,hl
            ld bc,72
            add hl,bc
drawCursorBorder:
            ld a,e
            sub 8
            ld d,a
            ld e,10
            ld bc,1
            jp uiInverseRectPixels
    ENDIF
; ---------------------------------------------------------------------------

    IF ENABLE_COLOR==0
drawCursor_0:
            ld    hl, 92D6h
            ld    b, 2
            call  activePanelPos
            jp    inverseRect
    ENDIF

