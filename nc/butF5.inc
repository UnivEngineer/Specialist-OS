;----------------------------------------------------------------------------
; MXOS NC.COM
; Клавиша F5 - копирование
;
; 2013-12-18 Дизассемблировано vinxru
;----------------------------------------------------------------------------

but_F5: ; Рисуем окно
        call    drawWindowIfSel

        ; Выводм "COPYING FROM A: TO"
        CENTER_LINE 15, 5Eh
        ld      (bios_vars.cursorY), hl
        ld      hl, aCopyFromTo 
        call    bios_printString
        call    printSelDrive

        call    inputForCopyMove
        cp      1Bh
        jp z,   mainReload

        call    copyFileInt
        jp      mainReload

