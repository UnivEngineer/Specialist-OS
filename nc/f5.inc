;----------------------------------------------------------------------------
; MXOS NC.COM
; Клавиша F5 - копирование
;
; 2013-12-18 Дизассемблировано vinxru
;----------------------------------------------------------------------------

F5:		; Рисуем окно
		call    drawWindowIfSel

		; Рисуем надпись "COPYING FROM A: TO"
        ; центр экрана это x = 60h
        ; половина ширина строки w = 15*3/2 = 22
        ; координата строки x = 60h-22 = 4Ah
		lxi     h, 4A5Eh
		shld    v_cursorY
		lxi     h, aCopyFromTo 
		call    printString
		call    printSelDrive

		call    inputForCopyMove
		cpi     1Bh
		jz      mainReload

		call    copyFileInt
		jmp     mainReload

