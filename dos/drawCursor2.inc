;+---------------------------------------------------------------------------
; MXOS
; Рисование курсора на экране (продолжение)
;
; 2013-12-12 Дизассемблировано vinxru
;----------------------------------------------------------------------------

drawCursor_1:     ; Цикл
            dec   a
            jp nz,  drawCursor_0

            ; b = vars.cursorCfg & 0xF
            ld    a, b
            and   0Fh
            ld    b, a

            ; Вывод курсора (b - высота, de - адрес на экране, hl - курсор)

drawCursor_2:     ; Повторить код ниже b раз
            dec   b
            jp M, popa_ret_2

            ; *de--     ^= hl;
            ld    a, (de)
            xor   h
            ld    (de),a
            inc   d
            ld    a, (de)
            xor   l
            ld    (de),a
            dec   d
            dec   e
            jp    drawCursor_2

