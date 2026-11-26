;+---------------------------------------------------------------------------
; MXOS
; Получить код нажатой клавиши
;
; выход:
;  a - код
;
; 2013-12-12 Дизассемблировано vinxru
;----------------------------------------------------------------------------

keyScan2:   ; Режим чтения ряда
            call    ioKeyMode82

            ; Одним чтением првоеряем все клавиши
            ld      a, (IO_KEYB_B)

            ; Эти биты не используются в сканировании
            or      3

            ; Ни одна клавиша не нажата
            cp      0FFh
            ret z

            ; Сохраняем регистры
            push    hl
            push    bc
            push    de

            ; Перебираем 12 столбцов кнопок
            ld      hl, 0FFEh
            ld      de, v_keybTbl + 11
            ld      b, 0FFh
loc_C1DD:   ld      a, h
            ld      (IO_KEYB_C),a
            ld      a, l
            ld      (IO_KEYB_A),a
            rrca
            cpl
            and     4
            ld      c, a
            ld      a, (IO_KEYB_B)
            or      c

            ; Перебираем 6 кнопок в столбце
            ld      c, 6
            rrca
            rrca
loc_C1F2:   rrca
            call    nc,    keybScan3
            dec     c
            jp nz,  loc_C1F2

            ; Цикл
            dec     de
            add     hl, hl
            inc     hl
            ld      a, h
            add     0F0h
            ld      h, a
            jp c,   loc_C1DD

            ; Режим ВВ55 по уполчанию
            call    ioKeyMode82

            ; Результат
            ld      a, b

            ; Восстаналвиаем регистры
            pop     de
            pop     bc
            pop     hl
            ret
