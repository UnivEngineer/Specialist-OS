;+---------------------------------------------------------------------------
; MXOS
; Ввод строки
;
; На входе
;  hl - начало буфера
;  de - конец буфера
;
; На выходе
;  bc, de, hl - сохраняются
;
; 2013-12-12 Дизассемблировано vinxru
;----------------------------------------------------------------------------

input:      ; Сохраняем регистры
            push  hl
            push  bc
            push  af

            ; Сохраняем значения
            ld    (v_input_start), hl
            ex    de, hl
            ld    (v_input_end), hl

            ; Помещаем в конец буфера 0
            ld    h, d
            ld    l, e
            ld  (hl), 0

input_loop: ; Ждем клавиашу
            call  j_getch
            ld    c, a

            ; Нажата служебная клавиша
            cp    20h
            jp c, input_spec

            ; Нажата клавиша Back space
            cp    7Fh
            jp z, input_bkspc

            ; ...
            ;nop
            ;nop
            ;nop

            ; Это конец буфера
            push  hl
            ld    hl, (v_input_end)
            call  cmp_hl_de_2
            pop   hl
            jp nz,  loc_C8DE          
            call  cmp_hl_de_2
            jp z, input_loop
            ld    a, (de)
            dec   de
            ld    (de),a
loc_C8DE:   push  bc
            ld    b, h
            ld    c, l
            inc   bc
            call  memmove_bc_hl
            pop   bc
            ld  (hl), c       ; *hl++     = c;
            call  j_printChar ; Вывод     символа     на экран
            inc   hl
            inc   de          
loc_C8EC:   push  hl
            ld    hl, (bios_vars.cursorY)
            ex    (sp), hl
            push  hl
            call  j_printString
            ld    c, ' '
            call  j_printChar
            pop   hl
            ex    (sp), hl
            ld    (bios_vars.cursorY), hl
            pop   hl

            ; Продолжение
            jp    input_loop

; ---------------------------------------------------------------------------

input_bkspc:      ; Если курсор в начале строки ничего не удаляем
            call  cmp_hl_de_2
            jp z, input_loop

            ; Сдвигаем строку
            ld    b, h
            ld    c, l
            inc   hl
            call  memmove_bc_hl

            ; Уменьшаем положение курсора
            dec   hl

            dec   de
            jp    loc_C8EC

; ---------------------------------------------------------------------------

input_spec: ; Нажато влево
            cp    8
            jp z, input_left

            ; Нажато вправо
            cp    18h
            jp z, input_right

            ; Нажат не ввод
            cp    0Dh
            jp nz,  input_loop

            ; Нажат ввод

            ; Сохраняем в конец стрки 0D
            ld    (de),a

            ; Восстанавливаем регистры и выходим
            pop   af
            pop   bc
            pop   hl
            ret

; ---------------------------------------------------------------------------

input_left: ; Если курсор в начале строки (hl==v_input_start) не перемещаем курсор
            ex    de, hl
            push  hl          
            ld    hl, (v_input_start)
            call  cmp_hl_de_2
            pop   hl
            ex    de, hl
            jp z, input_loop

            ; Уменьшаем положение курсора
            dec   hl

input_lr:   ; Перемещаем курсор вправо/влево
            call  j_printChar

            ; Продолжаем ввод
            jp    input_loop

; ---------------------------------------------------------------------------

input_right:      ; Если курсор в конце строк (hl==de) не перемещаем курсор
            call  cmp_hl_de_2
            jp z, input_loop

            ; Увеличиваем положение курсора
            inc   hl

            ; Общее продолжение
            jp    input_lr

