;+---------------------------------------------------------------------------
; MXOS
; Запись программы на ленту
;
; На входе
;  hl - начальный адрес
;  de - конечный адрес
;
; На выходе
;  bc - контрольная сумма
;  de, hl - сохраняются
;
; 2013-12-12 Дизассемблировано vinxru
;----------------------------------------------------------------------------

tapeSave:   ; Расчитываем CRC
            push    hl
            call    bios_calcCS
            pop     hl

            push    hl
            push    bc

            ; Пилот-тон (256 нулей)
            ld      b, 0
loc_C9A4:   xor     a
            call    tapeWrite
            dec     b
            jp nz,  loc_C9A4

            ; Стартовый байт 0E6h
            ld      a, 0E6h
            call    tapeWrite

            ; Запись адреса первого байта
            call    tapeWriteWord
            ex      de, hl

            ; Запись адреса последнего байта
            call    tapeWriteWord
            ex      de, hl
            
loc_C9B9:   ; Запись блока памяти от hl до de
            ld      a, (hl)
            call    tapeWrite
            call    cmp_hl_de_2
            inc     hl
            jp nz,  loc_C9B9

            ; Запись CRC
            pop     hl
            call    tapeWriteWord

            ; Возвращаем CRC в регистре bc
            ld      b, h
            ld      c, l

            ; Восстанавливаем регистры и выходим
            pop     hl
            ret

