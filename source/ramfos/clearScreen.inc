;----------------------------------------------------------------------------
; RAMFOS
; Очистить экран
;
; 2013-11-01 Дизассемблировано vinxru
;----------------------------------------------------------------------------

clearScreen:	
    IF COLOR_SUPPORT
		; Цвет по умолчанию
		ld a, INIT_SCREEN_COLOR
		call	setColor
    ENDIF
clearScreen2:
		; Сохраняем SP
		ld hl, 0
		add hl, sp
		ld (v_oldSP), hl

		; Колонки
		ld b, 48

		; Байт для заливки экрана
		ld a, (v_inverse)
		ld d, a
		ld e, a

		; Заливаем экран. Начинаем с 383,240.
		ld hl, 0BFF0h
loc_CE1B:	ld sp, hl
		 ld c, 17h ; 230 строк
loc_CE1E:	push de
		  push de
		  push de
		  push de
		  push de
		  dec c
		 jp nz, loc_CE1E
		dec h
		dec b
		jp nz, loc_CE1B

		; Восстаналиваем SP
		ld hl, (v_oldSP)
		ld sp, hl

		; Дальше магия с переменными и строками состояния

		; При печати двойного символа 1F скрываем нижнюю строку
		ld hl, v_dblClear
		ld a, 1Fh
		cp (hl)
		call z, bottomLineHide ; Если вызвана, то на выходе A=0		
		ld (v_dblClear), a

		ld a, (v_topLineHeight)
		or a
		call z, clearTopLine2

		ld a, (v_screenHeight)
		cp 250
		call z, sub_CE57

loc_CE4B:	 ; cursorX = 0
		ld h, 0

		; cursorY = byte_D231 - 2
		ld a, (v_topLineHeight+1)
		dec a
		dec a
		ld l, a
		
saveCursorPos:	 ; Сохранить положение курсора
		ld (v_cursorPos), hl
		ret

; ---------------------------------------------------------------------------

sub_CE57:	ld de, 10h
		jp clearLine ; d - строка, e	- высота

; ---------------------------------------------------------------------------
