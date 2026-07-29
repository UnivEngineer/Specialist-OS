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

            ; Рассчиытваем координату X (точнее, адрес)
            ld    h, 90h + (P_FILE_LIST_X1 * 2 / 8)
            cp    FILE_LIST_SIZE/2
            jp c, drawCursor_1
            sub   FILE_LIST_SIZE/2
            ld    h, 90h + (P_FILE_LIST_X2 * 2 / 8)

drawCursor_1:
            ; Рассчиытваем координату Y (точнее, адрес)
            ; a = a * 10 + P_FILE_LIST_Y - 8
            add   a, a
            ld    c, a
            add   a, a
            add   a, a
            add   a, c
            add   P_FILE_LIST_Y - 8
            ld    l, a

            ; Ширина курсора в байтах, высота в пикселях
            ld    bc, (10 << 8) + 10

            ; Если правая панель, сдвигаем курсор на пол экрана
drawCursor_2:
            ld    a, (state.activePanel)
            and   a
            jp z, drawCursor_3
            ld    a, 18h
drawCursor_3:
            add   a, h
            ld    h, a

            ; Рисуем курсор (hl = адрес, b = ширина, c = высота)
            call  inverseRect
            ret

; ---------------------------------------------------------------------------

    IF ENABLE_COLOR==0
drawCursor_0:
            ld    hl, 92D6h
            ld    b, 2
            jp    drawCursor_2
    ENDIF

