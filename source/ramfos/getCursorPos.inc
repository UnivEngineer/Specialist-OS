;+---------------------------------------------------------------------------
; RAMFOS
; Стандартная точка входа F81E	
; Получить координаты курсора в символах
;
; На выходе
;  h - строка (Y)
;  l - столбец (X)
;  Сохраняет остальные регистры
;
; 2013-11-01 vinxru
;----------------------------------------------------------------------------                   

getCursorPos:	push af
		ld hl, -1

		; h = v_cursorPosY / 10
		ld a, (v_cursorPos)
getCursorPos_0:	inc h
		 sub 0Ah
		jp nc, getCursorPos_0

		; l = v_cursorPosX / 3
		ld a, (v_cursorPos+1)
getCursorPos_1:	inc l
		 sub 3
		jp nc, getCursorPos_1

		pop af
		ret
