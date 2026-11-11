;+---------------------------------------------------------------------------
; MXOS
; Вывод символа на экран (или принтер)
;
; На входе
;  с - символ
;
; На выходе
;  Все регистры сохраняются
;
; 2013-12-12 Дизассемблировано vinxru
;----------------------------------------------------------------------------

printChar:  push  af
            push  de
            push  bc
            push  hl
            call  printChar2
printCharRestore:
            pop   hl
            pop   bc
            pop   de
            pop   af
            ret


; Публичный Raw: те же сохранённые AF/BC/DE/HL, код в A, без разбора
; управляющих байтов. Общий пиксельный helper оставлен CALL/RET, поскольку
; строки входят в него напрямую без повторного сохранения регистров.
printCharReal:
printCharRaw:
            push af
            push de
            push bc
            push hl

            ld c,a
            call printCharRawHelper
            jp printCharRestore
