;----------------------------------------------------------------------------
; MXOS NC.COM
; Рисование окошек
;
; 2013-12-18 Дизассемблировано vinxru
;----------------------------------------------------------------------------

drawWindowIfSel:
        ; Получить кол-во файлов
        ld      hl, state.panelA_filesCnt
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
        ld      hl, 9F65h           ; x=120 y=101
        ld      bc, (18 << 8) + 11  ; w=144 h=11
        jp      inverseRect

; ---------------------------------------------------------------------------

drawWindow2:
        COLOR   COLOR_DIALOG

        ; Рисуем окно
        ld      hl, g_window2
        call    draw

        ; Рисуем прямоугольник
        ld      hl, 9E87h           ; x=112 y=135
        ld      bc, (20 << 8) + 11  ; w=160 h=11
        jp      inverseRect
