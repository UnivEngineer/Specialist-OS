;----------------------------------------------------------------------------
; RAMFOS
; Стандартная точка входа C806
; Чтение байта с лента
;
; 2013-11-01 Дизассемблировано vinxru
;----------------------------------------------------------------------------

tapeRead:	push hl
		push de
		push bc

		; Промежуточный адрес возврата
		ld hl, popa_ret
		push hl

		ld hl, (v_tape)
		ex de, hl
		ld hl, 0FFE1h

		ld b, 8
tapeRead_0:	 call	tapeWaitC3
		 ld a, c
		 cp d
		 jp c, tapeRead_0
		 call	tapeWaitC3
		 ld a, c
		 cp d
		jp c, tapeRead_0

tapeRead_1:	 call	tapeWaitC3
		 ld a, (v_tape+1)
		 cp c
		 ccf
		 ld a, d
		 rra
		 ld d, a
		 call	tapeWaitC3
		 dec b
		jp nz, tapeRead_1

		call	tapeWaitC3
		ld a, e
		xor 1
		ld (v_tape), a
		ld a, (v_tape+1)
		cp c
		ccf
		rla
		ld c, a
		ld a, d
		or a
		push af
		pop de
		ld a, e
		rra
		rra
		xor c
		rra
		ld a, d
		ret nc

tapeError:	ld a, 3Fh
		scf
		ret
