;+---------------------------------------------------------------------------
; MXOS
; Прокрутка экрана вниз
;
; На выходе
;   af, bc - сохраняются
;
; 2013-12-12 Дизассемблировано vinxru
;----------------------------------------------------------------------------

            ; Сохраняем регистры
            push    bc
            push    af
            ld      hl, 0
            add     hl,sp
            ld      (bios_vars.oldSP), hl

            ; Подготовка переменных
            ld      sp, 0C000h    ; Куда
            ld      h, 0BFh       ; Откуда
            ld      b, 48         ; Столбцов
loc_C741:   ld      c, 246 / 6    ; 246 строк
            ld      l, 245        ; Начинаем с 245 строки

loc_C745:   ; Копируем 6 байт из hl в sp
            ld      d, (hl)
            dec     hl
            ld      e, (hl)
            dec     hl
            push    de          
            ld      d, (hl)
            dec     hl
            ld      e, (hl)
            dec     hl
            push    de
            ld      d, (hl)
            dec     hl
            ld      e, (hl)
            dec     hl
            push    de

            ; Цикл
            dec     c
            jp nz,  loc_C745

            ; Сохраняем h
            ld      a, h
            
            ; Очищаем верхние 10 строк
            ld      hl, (bios_vars.inverse)
            push    hl
            push    hl
            push    hl
            push    hl
            push    hl

            ; Восстанавливаем h
            ld      h, a

            ; Следующий столбец
            dec     b
            jp nz,  loc_C741

            ; Восстанавлиаем регистры и выходим
            ld      hl, (bios_vars.oldSP)
            ld      sp, hl
            pop     af
            pop     bc
            ret

