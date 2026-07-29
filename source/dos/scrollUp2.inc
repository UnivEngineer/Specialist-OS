;+---------------------------------------------------------------------------
; MXOS
; Прокрутка экрана вверх (продолжение)
;
; 2013-12-12 Дизассемблировано vinxru
;----------------------------------------------------------------------------

scrollUp2:  ; Копируем 246 байт из sp в H0 
            ld    c, 246 / 6
            ld    l, 0

scrollUp2_0:      ; Копирование 6 байт из sp в hl
            pop   de
            ld  (hl), e
            inc   hl
            ld  (hl), d
            inc   hl
            pop   de
            ld  (hl), e
            inc   hl
            ld  (hl), d
            inc   hl
            pop   de
            ld  (hl), e
            inc   hl
            ld  (hl), d
            inc   hl

            ; Цикл
            dec   c
            jp nz,  scrollUp2_0

            ; Сохраняем h
            ld    a, h
            
            ; Заполняем 10 байт 
            ld    hl, (bios_vars.inverse)
            push  hl
            push  hl
            push  hl
            push  hl
            push  hl

            ; Следующий столбец (sp += 20)
            ld    hl, 14h
            add   hl,sp
            ld    sp, hl

            ; Восстанавливаем h
            ld    h, a

            ; Следующий столбец
            inc   h

            ; Цикл
            dec   b
            jp nz,  scrollUp2

            ; Восстановление регистров и выход
            ld    hl, (bios_vars.oldSP)
            ld    sp, hl
            pop   af
            pop   bc
            ret

