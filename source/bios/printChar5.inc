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
            ld    (bios_vars.koi7), a
            ret

; ---------------------------------------------------------------------------
; Обработка ESC+). Включение KOI-8

printChar_e29:
            xor   a
            ld    (bios_vars.koi7), a
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
            ld a,h
            cp FONT_ADDR_DIV_8 >> 8
            jp nz,printChar_linear
            ld a,l
            cp FONT_ADDR_DIV_8 & 0FFh
            jp nz,printChar_linear
            ; Старшие три бита выбирают 256-байтный диапазон.
            ; Отсутствующие диапазоны заменяются пробелом.
            ld a,c
            and 0E0h
            rrca
            rrca
            rrca
            rrca
            ld e,a
            ld d,0
            ld hl,FONT_RANGES_FIELD
            add hl,de
            ld e,(hl)
            inc hl
            ld d,(hl)
            ld a,d
            or e
            jp z,printChar_missing
            ld a,c
            and 01Fh
            add a,a
            add a,a
            add a,a
            ld l,a
            ld h,0
            add hl,de
            ld de,FONT_ADDR
            add hl,de
            ex de,hl
            jp printChar_de
printChar_missing:
            ld a,c
            cp 0A3h
            ld hl,FONT_YO_LOWER_FIELD
            jp z,printChar_special
            cp 0B3h
            ld hl,FONT_YO_UPPER_FIELD
            jp z,printChar_special
printChar_space:
            ld hl,FONT_RANGES_FIELD+2
printChar_special:
            ld e,(hl)
            inc hl
            ld d,(hl)
            ld a,d
            or e
            jp z,printChar_space
            ex de,hl
            ld de,FONT_ADDR
            add hl,de
            ex de,hl
            jp printChar_de
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

