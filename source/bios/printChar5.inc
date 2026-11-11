;+---------------------------------------------------------------------------
; MXOS
; Обработка служебных кодов при выводе символа (продолжение)
; Расчет адреса символа в знакогенераторе и адреса вывода на экран
;
; 2013-12-12 Дизассемблировано vinxru
;----------------------------------------------------------------------------

; ---------------------------------------------------------------------------
; Обработка кода 9. Табуляция

printChar_c9:
            ld    a, (bios_vars.cursorX)  
            add   a, 12 ; 24
            ld    (bios_vars.cursorX),a
            ret

; ---------------------------------------------------------------------------
; Вывод символа C из RAM-шрифта по физическому адресу HL.
; Курсор перемещается вправо на шесть пикселей.

printChar_alt:
            ld a,h
            cp FONT_ADDR >> 8
            jp nz,printChar_linear
            ld a,l
            or a
            jp nz,printChar_linear
            ; charGen уже проверен выше. Для скаляра не повторяем эту
            ; проверку в общем preValid: достаточно базы и эпохи таблицы.
            ld a,(fast6.preBase+1)
            or a
            jp z,printChar_pagedRotate
            ld a,(GLYPH_CACHE_EPOCH)
            ld hl,fast6.preEpoch
            cp (hl)
            jp z,scalarPre.draw
printChar_pagedRotate:
            ld a,024h
            call scalar6.selectLayout
            ld d,FONT_ADDR >> 8
            ld e,c
            call printChar_position
            jp scalar6.scalar6Draw
; C — код, HL — физический адрес линейной таблицы, DE — адрес глифа.
printChar_linear:
            ld a,023h
            call scalar6.selectLayout
            ex de,hl
            ld l,c
            ld h,0
            add hl,hl
            add hl,hl
            add hl,hl
            add hl,de
            ex de,hl
            jp printChar_de

printChar_de:
            call printChar_position
            jp scalar6.scalar6Draw
printChar_position:
            ; Как в standalone PAGED: вычисление адреса в горячем пути.
            ; При X >= 190 оставляем штатный перенос перед рисованием.
            ld hl,(bios_vars.cursorY)
            ld a,h
            cp 190
            jp nc,printChar_positionWrap
            rrca
            rrca
            and 3Fh
            add a,90h
printChar_positionReady:
            inc h
            inc h
            inc h
            ld (bios_vars.cursorY),hl
            ld b,a
            ld a,l
            sub 7
            ld c,a
            ret
printChar_positionWrap:
            ld c,10
            call calcCursorAd_0
            jp printChar_positionReady
