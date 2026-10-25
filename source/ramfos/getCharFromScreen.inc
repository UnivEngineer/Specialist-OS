;----------------------------------------------------------------------------
; RAMFOS
; Получить код символа на экране
;
; 2013-11-01 vinxru
;----------------------------------------------------------------------------                   

getCharFromScreen:
		push hl
		push de
		push bc

		; Адрес возврата
		ld hl, popa_ret
		push hl

		; Положение курсора в пикселях
		ld hl, (v_cursorPos)

		; Расчет адреса
		ex de, hl
		call	calcCharAddr
		ex de, hl
		
		; с - смещение в битах
		; hl - адрес с видеопамяти

		ld b, 8 ; Высота символа
		ld de, unk_D21A ; Сюда будет скопирован символ с экрана
		push de
loc_CAF9:	push hl
		  ; Загрузить в HL из памяти по адресу HL
		  ld a, (hl)
		  inc h
		  ld l, (hl)
		  ld h, a
		  ; Сдвигаем HL на c*2 бит
		  ld a, c
loc_CAFF:	push af
		    call	hl_div_2
		    call	hl_div_2
		   pop af
		   dec a
		  jp nz, loc_CAFF
		  ; Сохраняем только нижние 6 бит по адресу de
		  ld a, l
		  and 3Fh
		  ld (de), a
		  dec de
		 pop hl
		 dec l
		 dec b
		jp nz, loc_CAF9

		; Если v_inverse не ноль
		ld a, (v_inverse)
		or a
		jp z, loc_CB29
		 ; Инвертируем байт по адресу de
		 ld b, 8
loc_CB1F:	   ; de++
		  inc de
		  ; *de = (*de ^ 0xFF) & 0x3F;
		  ld a, (de)
		  cpl
		  and 3Fh
		  ld (de), a
		  ; Цикл
		  dec b
		 jp nz, loc_CB1F
loc_CB29:	pop de

		; de - адрес на экране

		; Сравниваем каждый символ из знакогенератора с экраном

		ld a, -1
		ld hl, (v_charGenPtr) ; Знакогенератор
		ld bc, 8 ; высота символа
loc_CB32:	  ; Следующий символ
		 add hl, bc
		 push bc
		 push de
		 push hl
		  ld b, a
		  call	compareChar
		  ld a, b
		 pop hl
		 pop de
		 pop bc
		jp nz, loc_CB32

		; Знакогенератор не содержит символов 80-BF
		cp 80h
		ret c
		add a, 40h
		ret

; ---------------------------------------------------------------------------

compareChar:	 ; if(a == 0xBF) return true;
		cp 0BFh
		ret z
		; Увеличивае номер символа
		inc b
		; Далее memcmp_hl1_de_c
