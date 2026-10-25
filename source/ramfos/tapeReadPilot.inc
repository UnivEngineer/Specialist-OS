;----------------------------------------------------------------------------
; RAMFOS
; Стандартная точка входа C824
; Чтение пилот тона с ленты
;
; 2013-11-01 Дизассемблировано vinxru
;----------------------------------------------------------------------------

tapeReadPilot:	push hl
		push de
		push bc

		; Промежуточный адрес возврата
		ld hl, popa_ret
		push hl

		; Порт магнитофона
		ld hl, IO_KEYB_B

		ld e, 1
loc_C678:	ld d, 0
loc_C67A:	ld b, c
		 call	tapeWaitC3
		 ld a, c
		 sub b
		 jp nc, loc_C684
		  cpl
loc_C684:	cp 3
		jp nc, loc_C678
		dec d
		jp nz, loc_C67A

		call	tapeWaitC3

		; Повторить D раз
		ld bc, 100h
loc_C693:	 call	tapeWait
		 inc bc
		 inc bc
		 dec d
		jp nz, loc_C693

		ld l, c
		ld h, b
		or a

		; bc >>= 1
		ld a, b
		rra
		ld b, a
		ld a, c
		rra
		ld c, a

		add hl, bc
		ld l, e
		ld (v_tape), hl
		ret
