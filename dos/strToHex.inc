;+---------------------------------------------------------------------------
; MXOS
; Преобразвоние строки в 16-ричное число
; Заимствовано из RAMFOS
;
; На входе
;  de - Строка
;
; На выходе
;  bc       - Сохраняется
;  de       - Следующий символ за запятой или концом строки
;  jz       - Ошибка
;  jnz, jc  - Конец строки
;  jnz, jnc - Пробел или запятая
;  hl       - Число
;
; 2013-11-01 Дизассемблировано vinxru
;----------------------------------------------------------------------------

strToHex:
            ld  hl, 0       

strToHex_loop:    ; Чтение символа
            ld    a, (de)
            inc   de

            ; Если конец строки, выходим с флагами NZ, c
            cp    0Dh
            jp z, ora_a_stc_ret

            ; Если пробел или запятая, выходим с флагами NZ, NC
            cp    ','
            jp z, ora_a_ret         
            cp    ' '
            jp z, ora_a_ret

            ; Если некорректный символ, выходим с флагами С,Z
            sub   '0'
            jp M, xra_a_stc_ret           

            ; Если это цифра
            cp    10
            jp M, strToHex_0

            ; Если некорректный символ, выходим с флагами С,Z
            cp    11h
            jp M, xra_a_stc_ret
            cp    17h
            jp p, xra_a_stc_ret

            ; Это буквы A..F
            sub   'A'-'0'-10

strToHex_0: 
            ; hl *= 16
            add   hl, hl
            add   hl, hl
            add   hl, hl
            add   hl, hl

            ; Если переполнение, выходим c флагами С,Z
            jp c, xra_a_stc_ret
            
            ; hl += bc
            push bc
            ld   b, 0
            ld   c, a
            add  hl, bc
            pop  bc

            jp    strToHex_loop

xra_a_stc_ret:    ; Вывод знака вопроса
            ld  c, '?'
            call      printChar
            xor a

ora_a_stc_ret:
            or  a
            scf
            ret

ora_a_ret:
            or  a
            ret
