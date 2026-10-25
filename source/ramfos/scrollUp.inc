;----------------------------------------------------------------------------
; RAMFOS
; Сдвиг экрана вверх
;
; 2013-11-01 Дизассемблировано vinxru
;----------------------------------------------------------------------------

scrollUp:	push hl
		ld a, l ; c =  (l + 2 -	word_D231)/2

		ld hl, (v_topLineHeight)
		ld e, h ; e = word_D231
		inc a
		inc a
c_scrolUpEx:	sub e
		rra
		ld c, a
		; Адрес видео
		ld h, 90h
		ld d, h
		; Колонки		
		ld b, 48
loc_CD5A:	push hl
		 push de
		 push bc
		  ; Копируем C байт
		  inc c
		  dec c
		  call nz, memcpy_hl_de_c2
		  ; Очищаем последние 10 байт
		  ld a, (v_inverse)
		  ld c, 10
loc_CD67:	ld (hl), a
		   inc l
		   dec c
		  jp nz, loc_CD67
		 pop bc
		 pop de
		 pop hl
		 ; Следующая колонка
		 inc h
		 inc d
		 dec b
		jp nz, loc_CD5A
		pop hl
		; Установка положения курсора	
saveCursorPosX0:	ld h, 0
		jp saveCursorPos
