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
            ld hl,fontRanges
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
            ex de,hl
            jp printChar_de
printChar_missing:
            ; Ё/ё в KOI8-R лежат вне пяти основных диапазонов.
            ld a,c
            cp 0A3h
            ld de,fontYoLower
            jp z,printChar_de
            cp 0B3h
            ld de,fontYoUpper
            jp z,printChar_de
            ld de,FONT_ADDR
            jp printChar_de
fontYoLower:
            INCBIN "../../assets/system-rom/FONT.FNT", 0A3h*8, 8
fontYoUpper:
            INCBIN "../../assets/system-rom/FONT.FNT", 0B3h*8, 8
fontMissing = 0
fontRanges:
            DW fontMissing, FONT_ADDR, FONT_ADDR+0100h, FONT_ADDR+0200h
            DW fontMissing, fontMissing, FONT_ADDR+0300h, FONT_ADDR+0400h
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

