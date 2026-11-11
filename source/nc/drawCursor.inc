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
            ld    h, 90h + 1
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

            ; Одинаковая ширина выделения в обоих столбцах: 80 пикселей.
            ld    bc,(10 << 8) + 10

            ; Если правая панель, сдвигаем курсор на пол экрана
drawCursor_2:
            call  activePanelPos

            jp    inverseRect
; ---------------------------------------------------------------------------

    IF ENABLE_COLOR==0
drawCursor_0:
            ld    hl, 92D6h
            ld    b, 2
            call  activePanelPos
            jp    inverseRect
    ENDIF

