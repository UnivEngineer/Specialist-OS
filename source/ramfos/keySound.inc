;+---------------------------------------------------------------------------
; RAMFOS
; Звуковой сигнал при нажатии клавиши
;
; 2013-11-01 vinxru
;----------------------------------------------------------------------------

keySound:	push bc
		 ; Частота
		 ex de, hl
		 ld hl, (v_beepConfig)
		 ex de, hl

		 ; Понижаем частоту в два раза, если нажата РУС/LAT
		 ld a, (v_rusLat)
		 and 1
		 ld a, d
		 jp nz, keySound_0
		  rrca ; Интересно, а вдруг CF?
keySound_0:	ld d, a

		 ; Устаналиваем таймер (звук)
		 ld b, e
		 ld c, e
		 or a
		 call	setTimer
		pop bc

		; Для функции keyScan
		ld a, 0FFh
		ret
