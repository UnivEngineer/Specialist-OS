;+---------------------------------------------------------------------------
; MXOS
; Рисование символа на экране
;
; На входе 
;  bc - адрес символа
;  de - адрес в видеопамяти
;  h  - координата X в пикселях / 2
;
; 2013-12-12 Дизассемблировано vinxru
;----------------------------------------------------------------------------

            ; Высота символа
            ld    l, 8

            ; Переходим к одной из функций вывода
            ld    a, h
            and   3
            jp z, drawChar0   ; a==0
            dec   a
            jp z, drawChar1   ; a==1
            dec   a
            jp z, drawChar2   ; a==2
            dec   a
            jp z, drawChar3   ; a==3
            ; Продолжение на drawChar2

; ---------------------------------------------------------------------------

drawChar2:  ld    a, (bc)            
            and   0FCh
            ld    h, a
            ld    a, (de)            
            rrca
            rrca
            rrca
            rrca
drawChar2_cma:
            cpl
            push  af
            and   3
            xor   h
            ld    (bc), a
            inc   b
            ld    a, (bc)
            and   0Fh
            ld    h, a
            pop   af
            and   0F0h
            xor   h
            ld    (bc),a
            dec   b

            ; Цикл
            inc   bc
            inc   de
            dec   l
            jp nz,  drawChar2
            ret

; ---------------------------------------------------------------------------

drawChar3:  ld    a, (bc)
            and   3
            ld    h, a
            ld    a, (de)
            rlca
            rlca
drawChar3_xri:
            xor   0FCh
            xor   h
            ld    (bc),a

            ; Цикл
            inc   de
            inc   bc
            dec   l
            jp nz,  drawChar3
            ret

; ---------------------------------------------------------------------------

drawChar0:  ld    a, (bc)
            and   0C0h
            ld    h, a
            ld    a, (de)
drawChar0_xri:
            xor   3Fh
            xor   h
            ld    (bc),a

            ; Цикл
            inc   de
            inc   bc
            dec   l
            jp nz,  drawChar0
            ret

; ---------------------------------------------------------------------------

drawChar1:  ld    a, (bc)
            and   0F0h
            ld    h, a
            ld    a, (de)
            rrca
            rrca
drawChar1_cma:
            cpl
            and   0Fh
            xor   h
            ld    (bc),a
            inc   b
            ld    a, (bc)
            and   3Fh
            ld    h, a
            ld    a, (de)
            rrca
            rrca
drawChar1_cma2:
            cpl
            and   0C0h
            xor   h
            ld    (bc),a
            dec   b

            ; Цикл
            inc   bc
            inc   de
            dec   l
            jp nz,  drawChar1
            ret

; ---------------------------------------------------------------------------
; Вызывается из printChar_real2

normalVideo:
            ; Меняем CZ normalVideo на CNZ inverseVideo
            ld    hl, inverseVideo
            ld    (printChar_poly+1), hl
            ld    a, 0C4h
            ld    (printChar_poly),a

            ; Аргумент XRI
            xor   a
            ld    (drawChar0_xri+1),a
            ld    (drawChar3_xri+1),a
normalVideo_1:
            ; Меняем CMA на NOP
            ld    (drawChar2_cma),a       
            ld    (drawChar1_cma),a
            ld    (drawChar1_cma2),a
            ret

; ---------------------------------------------------------------------------
; Вызывается из printChar_real2

inverseVideo:
            ; Меняем CNZ inverseVideo на CZ normalVideo
            ld    hl, normalVideo
            ld    (printChar_poly+1), hl
            ld    a, 0CCh           
            ld    (printChar_poly),a

            ; Меняем аргумент XRI
            ld    a, 0FCh
            ld    (drawChar3_xri+1),a
            ld    a, 3Fh
            ld    (drawChar0_xri+1),a

            ; Меняем NOP на CMA
            ld    a, 2Fh
            jp    normalVideo_1

