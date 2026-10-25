;+---------------------------------------------------------------------------
; RAMFOS
; Включение/выключение эхопечати на принтер
; Вызывается при нажатии на соответствующую клавишу
;
; 2013-11-01 Дизассемблировано vinxru
;----------------------------------------------------------------------------

    IF NO_PRINTER == 0

printerEchoMode:	ld hl, v_printerEcho
		ld a, 0D2h
		cp (hl)
		jp nz, printerEchoMode_0
		 xor a
printerEchoMode_0:
		ld (hl), a
		jp specialKeysRet
    ENDIF
