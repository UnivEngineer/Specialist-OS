;----------------------------------------------------------------------------
; RAMFOS
; Сдвиг экрана вниз
;
; 2013-11-01 Дизассемблировано vinxru
;----------------------------------------------------------------------------

scrollDownX0:	 ; x = 0
		ld h, 0 ; Влияет на расчет C, смотри ниже

scrollDown:	push hl
		; c = cursorY - 8
		ld a, l
		sub 8
		; if(v_cursorX != 0) c += 10
		dec h
		inc h
		jp z, scrollDown_0
		  add a, 10
scrollDown_0:	ld c, a

		; l = v_screenHeight, e = h = v_screenHeight-10
		ld hl, (v_screenHeight)
		ld e, h

		; if(v_screenHeight < c) goto CDF9
		ld a, e
		sub c
		jp c, scrollDown_1

		; с = (v_screenHeight - c) / 2
		rra
		ld c, a

		; Адрес на экране. Для DE, HL идентичный, поскольку будет отличаться E, L
		ld h, 90h
		ld d, h

		; 48 столбцов
		ld b, 48
scrollDown_2:	push hl
		 push de
		 push bc
		  ; Копируем C линий
		  inc c
		  dec c
		  call nz, memcpyb_hl_de_c2
		  ; Верхние 10 линий очищаем
		  ld a, (v_inverse)
		  ld c, 10
scrollDown_3:	dec l
		   ld (hl), a
		   dec c
		  jp nz, scrollDown_3
		 pop bc
		 pop de
		 pop hl
		 ; Следующий столбец
		 inc h
		 inc d
		 dec b
		jp nz, scrollDown_2

scrollDown_1:	 ; Восстанавливам L, он же cursorY
		; cursorX, он же H будет заменен 0 в функции saveCursorPosX0
		pop hl
		jp saveCursorPosX0
