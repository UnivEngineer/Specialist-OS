;----------------------------------------------------------------------------
; MXOS DOS.SYS
; Вывод десятичного числа из de
; Перенесено из NC.COM
;
; вход:
;   de = число
;   b = дополнить ведущими символами: 20h - пробелами, 30h - нулями, 00h - не дополнять
;
; выход:
;   hl, de, bc - сохраняются
;
; 2013-12-18 Дизассемблировано vinxru
;----------------------------------------------------------------------------

printDecWord:
            push  de
            push  bc
            push  hl
            ex    de, hl
            ; ld    b,  20h  ; b = маска кода символа, заменяет ведущие нули на (код_символа and маска)
            ld    de, -10000
            call  printDec1
            ld    de, -1000
            call  printDec1
            ld    de, -100
            call  printDec1
            ld    de, -10
            call  printDec1
            ld    a, l
            add   a, '0'
            ld    c, a
            call  printChar
            pop   hl
            pop   bc
            pop   de
            ret

; ---------------------------------------------------------------------------


printDec1:  ; c = '0' + (hl / de), hl /= de;
            ld    c, '0'-1

loc_DB11:   inc   c
            add   hl, de
            jp c, loc_DB11

            ld    a, d        ; hl -= de;
            cpl
            ld    d, a
            ld    a, e
            cpl
            ld    e, a
            inc   de
            add   hl, de
            ld    a, c

            ; if (c!='0') b=0xFF; c&=b;          
            cp    '0'
            jp z, loc_DB26
            ld    b, 0FFh   ; первый не ноль, меняем маску кода символа на 0FFh
loc_DB26:   and   b
            ret   z ; выходим если код символа 0
            ld    c, a
            jp    printChar

