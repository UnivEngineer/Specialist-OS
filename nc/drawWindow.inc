;----------------------------------------------------------------------------
; MXOS NC.COM
; Рисование окошек
;
; 2013-12-18 Дизассемблировано vinxru
;----------------------------------------------------------------------------

drawWindowIfSel:; Получить кол-во файлов
        ld      hl, panelA_filesCnt
        call    readBytePanel

        ; Если файлов нет, выходим
        and     a
        pop     hl ; Адрес возврата
        jp z,   mainDriveChanged
        push    hl
            

; ---------------------------------------------------------------------------

drawWindow1:
        COLOR   COLOR_DIALOG

        ; Рисуем окно
        ld      hl, g_window1
        call    draw
      
        ; Рисуем прямоугольник
        ld      hl, 9F66h   ; x=120 y=102
        ld      b, 18       ; w=144 h=9
        jp      inverseRect

; ---------------------------------------------------------------------------

drawWindow2:
        COLOR   COLOR_DIALOG

        ; Рисуем окно
        ld      hl, g_window2
        call    draw

        ; Рисуем прямоугольник
        ld      hl, 9E88h   ; x=112 y=136
        ld      b, 20       ; w=160 h=9
        jp      inverseRect
