;----------------------------------------------------------------------------
; MXOS NC.COM
;  лавиша F5 - копирование
;
; 2013-12-18 ƒизассемблировано vinxru
;----------------------------------------------------------------------------

F5:     ; –исуем окно
        call    drawWindowIfSel

        ; Выводм "COPYING FROM A: TO"
        ; центр экрана это x = 60h
        ; половина ширина строки w = 15*3/2 = 22
        ; координата строки x = 60h-22 = 4Ah
        ld      hl, 4A5Eh
        ld      (bios_vars.cursorY), hl
        ld      hl, aCopyFromTo 
        call    bios_printString
        call    printSelDrive

        call    inputForCopyMove
        cp      1Bh
        jp z,   mainReload

        call    copyFileInt
        jp      mainReload

