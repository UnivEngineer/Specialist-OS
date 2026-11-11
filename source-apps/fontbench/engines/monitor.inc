; Штатное пиксельное ядро BIOS в коллекции standalone.
; Печатный корпус идёт локально; управляющие коды обслуживает BIOS.
    MODULE monitor
scalar:
    push af
    push bc
    push de
    push hl
    call body
    pop hl
    pop de
    pop bc
    pop af
    ret
body:
    IFNDEF FONT_ENGINE_DLL
    ld a,c
    cp 020h
    jp c,bios_printChar
    ld a,(bios_upper.koi7)
    inc a
    jp nz,mapped
    ld a,c
    cp 0C0h
    ret nc
    cp 060h
    jp c,mapped
    cp 080h
    jp nc,mapped
    add a,080h
    ld c,a
    ENDIF
mapped:
    ld a,(bios_vars.inverse)
    or a
printChar_poly:
    call z,normalVideo
    ld hl,(renderFont8)
    jp printChar_alt
calcCursorAddr:
    ld hl,(bios_vars.cursorY)
    ld a,h
    rrca
    rrca
    and 03Fh
    add a,090h
    ret
; Копия printChar5.inc из BIOS; изменения standalone независимы.
;+---------------------------------------------------------------------------
; MXOS
; Обработка служебных кодов при выводе символа (продолжение)
; Расчет адреса символа в знакогенераторе и адреса вывода на экран
;
; 2013-12-12 Дизассемблировано vinxru
;----------------------------------------------------------------------------

; ---------------------------------------------------------------------------
; Обработка ESC+(. Включение KOI-7

printChar_e28:
            ld    a, 0FFh
            ld    (bios_upper.koi7), a
            ret

; ---------------------------------------------------------------------------
; Обработка ESC+). Включение KOI-8

printChar_e29:
            xor   a
            ld    (bios_upper.koi7), a
            ret

; ---------------------------------------------------------------------------
; Обработка кода 9. Табуляция

printChar_c9:
            ld    a, (bios_vars.cursorX)  
            add   a, 12 ; 24
            ld    (bios_vars.cursorX),a
            ret

; ---------------------------------------------------------------------------
; Вывод символа С из знакогенартора по адресу hl*8 на экран в положение 
; курсора. Курсор при этом перемещается 

printChar_alt:
printChar_linear:
            ; de = (hl+c) * 8
            ld    b, 0
            add   hl, bc
            add   hl, hl
            add   hl, hl
            add   hl, hl
            ex    de, hl

            ; Продолжение в printChar_de

; ---------------------------------------------------------------------------
; Вывод символа de на экран в положение курсора. Курсор при этом перемещается 
; вправо.

printChar_de:
            ; Рассчитываем адрес вывода (hl=v_cursorPos, a*256+l=адрес)
            call  calcCursorAddr

            ; Перемещаем курсор вправо
            inc   h
            inc   h
            inc   h           
            ld    (bios_vars.cursorY), hl

            ; Корректируем адрес вывода (bc = al - 7)
            ld    b, a
            ld    a, l
            sub   7
            ld    c, a

            ; Продолжение в drawChar


drawEntry:
; Копия drawChar.inc из BIOS; изменения standalone независимы.
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


    ENDMODULE
