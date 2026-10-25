;+---------------------------------------------------------------------------
; RAMFOS
; Печать на принтере
;
; 2013-11-01 Дизассемблировано vinxru
;----------------------------------------------------------------------------

    IF NO_PRINTER == 0

printerXlat:	 ; c = v_printerXlat[c % 64];
		ld a, c
		and 63
		ld hl, v_printerXlat
		ld c, a
		ld b, 0
		add hl, bc
		ld c, (hl)

printer:	push af

		; Режим КР580ВВ55
		ld a, 90h
		ld (IO_EXT_MODE), a

		; Инвертируем и записываем данные
		ld a, c
		cpl
		ld (IO_EXT_B), a

printer_0:	  ; Пользователь может прервать ожидание
		 ld a, (IO_KEYB_B)
		 and 2
		 jp z, printer_break

		 ; Ждем пока 6-ой бит не нулевой
		 ld a, (IO_EXT_A)
		 and 40h
		jp nz, printer_0

		; Строб
		ld a, 3
		ld (IO_EXT_C), a

		; Пауза
		nop
		nop
		nop
		nop
		nop
		nop
		nop

		; Строб
		xor a
		ld (IO_EXT_C), a

printer_break:	pop af
		ret

		DB 0
    ELSE

printer:
printerHex:
printerEchoMode: 
doPrinterEcho:
		ret
    ENDIF
