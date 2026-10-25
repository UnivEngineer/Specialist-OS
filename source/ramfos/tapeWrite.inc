;+---------------------------------------------------------------------------
; RAMFOS
; Стандартная точка входа C80С
; Запись байта на магнитофон
;
; На входе
;  С - байт
;  A - укорочение первого импульса *15 тактов
;
; Сохраняет регистры BC,DE,HL
;
; 2013-11-01 Дизассемблировано vinxru
;----------------------------------------------------------------------------

tapeWrite:	push hl
		push bc
		 ld hl, (v_tapePulseB)
		 add a, 3
		 cpl
		 add a, l
		 ld l, a
		 call	tapePulse
		 or a
		 or a
		 ld b, 8
tapeWrite_0:	  call	tapePulseBit
		  dec b
		 jp nz, tapeWrite_0
		 call	tapeWriteEnd
		 push hl
		 push hl
		  call	tapePulseA
		 pop hl
		 pop hl
		pop bc
		pop hl
		ret

; ---------------------------------------------------------------------------

tapePulseBit:	ld a, c
		rrca
		ld c, a
		jp c, tapePulseA
tapePulseB:	ld hl, (v_tapePulseB)
		jp tapePulse

; ---------------------------------------------------------------------------

tapeWriteEnd:	ld a, c
		or a
		ld c, a
		jp po, tapePulseB
		ld hl, (v_tapePulseA)
		dec l
		call	tapePulse
tapePulseA:	ld hl, (v_tapePulseA)
		jp tapePulse
