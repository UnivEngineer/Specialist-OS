;+---------------------------------------------------------------------------
; MXOS
; Копирование накладывающихся блоков памяти (с ошибкой)
;
; На входе
;  hl - откуда
;  de - откуда, конечный адрес не включая
;  bc - куда, с увеличением адресов
;
; 2013-12-12 Дизассемблировано vinxru
;----------------------------------------------------------------------------

memmove_bc_hl:    ; Сохраняем регистры
            push  hl
            push  bc
            push  de

            ; Если b<h или c<l, то
            ld    a, b
            cp    h
            jp c, memcpy_bc_hl2
            ; Тут не хватает jnz
            ld    a, c
            cp    l
            jp c, memcpy_bc_hl2

            ; bc = bc + de - hl
            push  hl          
            call  sbb_de_hl_to_hl
            add   hl, bc
            ld    b, h
            ld    c, l
            pop   hl

memcpyb_bc_de:    ; Копируем из de в bc с уменьшением адресов, пока hl не равно de
            ld    a, (de)
            ld    (bc),a
            call  cmp_hl_de_2
            dec   de
            dec   bc
            jp nz,  memcpyb_bc_de

            ; Восстановление регистров и выход
            jp    popDBH_ret2

; ---------------------------------------------------------------------------

memcpy_bc_hl2:    ; Копируем из hl в bc с увеличением адресов, пока hl не равно de
            ld  a, (hl)
            ld    (bc),a
            call  cmp_hl_de_2
            inc   hl
            inc   bc
            jp nz,  memcpy_bc_hl2

popDBH_ret2:      ; Восстановление регистров и выход
            pop   de
            pop   bc
            pop   hl
            ret

