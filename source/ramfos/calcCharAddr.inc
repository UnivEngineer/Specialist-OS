;----------------------------------------------------------------------------
; RAMFOS
; Расчет ареса символа в видеопамяти памяти и знакогенераторе
;
; 2013-11-01 Дизассемблировано vinxru
;----------------------------------------------------------------------------

calcCharAddr2:	 ; Какая то хрень
		ld b, c
calcCharAddr3:	ld (v_dblClear), a

		; hl = charGen + c * 8
		push de
		 ld hl, (v_charGenPtr)
		 ex de, hl
		 ld l, c
		 ld h, 0
		 add hl, hl
		 add hl, hl
		 add hl, hl
		 add hl, de
		pop de

; Получить адрес и смещение символа на экране по координатам
; Вход: de - координаты
; Выход: de - адрес, c - смещение
		
calcCharAddr:	 ; c = (d & 3)
		ld a, d
		and 3
		ld c, a
		ld a, 5
		sub c
		ld c, a
	
		; d = d / 4 + 0x90		
		ld a, d
		and 0FCh
		rrca
		rrca
		add a, 90h
		ld d, a
		ret
