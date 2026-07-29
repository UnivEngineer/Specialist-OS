;+---------------------------------------------------------------------------
; MXOS
; Рисование курсора на экране
;
; На выходе
;  bc, de, hl - сохраняются
;
; 2013-12-12 Дизассемблировано vinxru
;----------------------------------------------------------------------------

drawCursor: ; Если установлен 7-ой бит bios_vars.cursorCfg & 0x80, выходим
            ld    a, (bios_vars.cursorCfg)
            and   a
            ret m 

drawCursor2:      ; Курсор нарисован/стерт
            inc   c
            
drawCursor3:      ; Сохраняем регистры
            push  hl
            push  bc
            push  de

            ; Рассчитываем адрес на экране
            call  calcCursorAddr
            ld    d, a

            ; Положение курсора от +2 до -5 (при битах 654 от 0 до 7)
            ; b = bios_vars.cursorCfg
            ; e = ~((bios_vars.cursorCfg / 16) % 8 - 3) + l
            ld    a, (bios_vars.cursorCfg)
            ld    b, a
            rrca
            rrca
            rrca
            rrca
            and   7
            sub   3
            cpl
            add   a, l
            ld    e, a

            ; Рассчитыаем маску курсора
            ;  hl = 011111100b << (((~h) % 4 +1)*2)
            ld    a, h
            cpl
            and   3
            ld    hl, 011111100b ; FCh
            inc   a
drawCursor_0:     add   hl, hl
            add   hl, hl
            jp    drawCursor_1

