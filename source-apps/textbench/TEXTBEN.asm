; Одинаковый цикл для основного ОЗУ и страницы 0 ДОЗУ.
; Запуск стендом: DE = число строк, SP = 8000h, прерывания запрещены.
; CALL заменяется стендом на BIOS или общий шлюз; остальной код одинаков.
            include "../../source/include/mxos.inc"
            org 0100h
entry:      di
start:      ld hl, text
nextChar:   ld a, (hl)
            or a
            jp z, nextLine
            ld c, a
output:     call bios_printChar
            inc hl
            jp nextChar
nextLine:   dec de
            ld a, d
            or e
            jp nz, start
done:       jp done
; Стенд записывает сюда строку (до 47 знаков), CR, LF и нуль.
text:       defs 50, 0
            assert $ < 08000h
