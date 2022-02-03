;----------------------------------------------------------------------------
; MXOS NC.COM
; Ввод строки
;
; 2013-12-18 Дизассемблировано vinxru
;----------------------------------------------------------------------------

input:      ; Вывод строки
            push    hl
            ld      hl, nc_vars.input
            call    printStringInv
            pop     hl

            ; Включаем инверсию
            ld      a, 1
            ld      (bios_vars.inverse),a

input_loop: ; Ввод символа
            call  bios_getch

            ; Enter или ESC
            cp      0Dh
            jp z,   input_exit
            cp      1Bh
            jp z,   input_exit

            ; Если служебная клавиша или пробел, то пропускаем
            cp      21h
            jp c,   input_loop

            ; Сохраняем для вывода
            ld      c, a

            ; Помещаем символ в буфер
            ld      (de),a

            ; Удаление
            cp      7Fh
            jp z,   input_bkspc

            ; Если переполнение
            ld      a, (nc_vars.input + P_INPUT_WIDTH) & 0FFh  ; a = младший байт адреса пследнего байта буфера
            cp      e
            jp z,   input_loop

            ; Вывод символа на экран            
            call    bios_printChar

            ; Увеличение длины строки
            inc     de

            jp      input_loop

; ---------------------------------------------------------------------------

input_bkspc:
            ; Если это начало строки, выходим
            ld      a, nc_vars.input & 0FFh
            cp      e
            jp z,   input_loop

            ; Уменьшаем длину строки
            dec     e

            ; Стираем символ с экрана
            push    hl
            ld      hl, asc_DC17
            call    bios_printString
            pop     hl

            jp      input_loop

; ---------------------------------------------------------------------------

input_exit:
            ; Сохраняем ESC или Enter для выхода 
            ld      c, a

            ; Записываем в конец строки 0Dh, 0
            ex      de, hl
            ld      (hl), 0Dh
            inc     hl
            xor     a
            ld      (hl), a

            ; Восстанавливаем инверсию
            ld      (bios_vars.inverse),a

            ; Выход           
            ld      a, c
            ret

