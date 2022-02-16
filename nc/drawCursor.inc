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

            ; Рассчиытваем координату X (h = a < 18 ? 0x91 : 0x9D)
            ld    h, 91h
            cp    18
            jp c, drawCursor_1
            sub   18
            ld    h, 9Dh

drawCursor_1:
            ; Рассчиытваем координату Y (l = a * 10 + 18h)
            add   a, a
            ld    c, a
            add   a, a
            add   a, a
            add   a, c
            add   a,24
            ld    l, a

            ; Ширина
            ld    b, 10

            ; Если правая панель, сдвигаем курсор на пол экрана
drawCursor_2:     ld    a, (state.activePanel)
            and   a
            jp z, drawCursor_3
            ld    a, 18h
drawCursor_3:     add   a, h
            ld    h, a

            ; Рисуем курсор (hl - адрес, b - ширина, высота 9)
            call  inverseRect
            ret

; ---------------------------------------------------------------------------

    IF ENABLE_COLOR==0
drawCursor_0:     ld    hl, 92D6h
            ld    b, 2
            jp    drawCursor_2
    ENDIF

