;+---------------------------------------------------------------------------
; RAMFOS
; Стандартная точка входа C851
; Воспроизвести звуковой сигнал через ВВ55
;
; Частота задается в v_oldBeep
;
; 2013-11-01 Дизассемблировано vinxru
;----------------------------------------------------------------------------

oldBeep:	push af
		push bc
		push hl
		 ld hl, (v_oldBeep)
oldBeep_0:	ld a, 0Bh
		  ld (IO_KEYB_MODE), a
		  call	delay_l
		  ld a, 0Ah
		  ld (IO_KEYB_MODE), a
		  call	delay_l
		  dec h
		 jp nz, oldBeep_0
		pop hl
		pop bc
		pop af
		ret
