;+---------------------------------------------------------------------------
; RAMFOS
; Функции перемещения курсора
;
; На входе в каждую функцию HL должен содержать v_cursorPos
;
; 2013-11-01 Дизассемблировано vinxru
;----------------------------------------------------------------------------

cursorRight:	 ; Если v_cursorX >= 190, то перейти на новую строку
		ld a, h
		cp 190
		jp nc, cursorCrLf

		; v_cursorX += 3
		add a, 3
		ld h, a

		; Сохранить новое положение курсора (shld v_cursorPos, ret)
		jp saveCursorPos

; ---------------------------------------------------------------------------

cursorCrLf:	 ; v_cursorX = 0
		ld h, 0

cursorDown:	 ; Если v_cursorY > screenHeight+10, сдвинуть экран вверх
		ld a, (v_screenHeight+1)
		cp l
		jp c, scrollUp

		; cursorY += 10
		ld a, l
		add a, 10
		ld l, a

		; Сохранить новое положение курсора (shld v_cursorPos, ret)
		jp saveCursorPos

; ---------------------------------------------------------------------------

cursorLeft:	 ; Если курсор у левого края экрана, ничего не делать
		ld a, h
		cp 2
		jp c, saveCursorPos

		; v_cursorX -= 3
		sub 3
		ld h, a

		; Сохранить новое положение курсора (shld v_cursorPos, ret)
		jp saveCursorPos

; ---------------------------------------------------------------------------

cursorUp:	 ; Если курсор не на верхней служебной строке, то сдвинуть экран вверх
		ld a, (v_topLineHeight+1)
		cp l
		jp nc, scrollDownX0

		; v_cursorY -= 10
		ld a, l
		sub 10
		ld l, a

		; Сохранить новое положение курсора (shld v_cursorPos, ret)
		jp saveCursorPos
