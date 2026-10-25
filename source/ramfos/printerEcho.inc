;+---------------------------------------------------------------------------
; RAMFOS
; Эхопечать на принтере
;
; 2013-11-01 Дизассемблировано vinxru
;----------------------------------------------------------------------------

    IF NO_PRINTER == 0

printerEcho_crLf:
		; В сумме код ниже заменяет 0A или 0D на пару 0A, 0D. 
		; Но в случае "0A любые символы 0D", послений не дублируется

		; Вместо 0A, то печатаем пару символов 0A, 0D.
		ld hl, v_printerCrLf
		cp 0Ah
		jp z, printerEcho_0

		; Вместо 0D, то печатаем пару символов 0A, 0D, только если прошлый перенос был 0D.
		; Иначе выводим 0D как есть, без второго сивола.
		cp 0Dh
		jp nz, printerEcho_spc
		cp (hl)

printerEcho_0:	ld c, 0Ah
		call z, printer
		ld c, 0Dh
		ld (hl), a
		jp printer

; ---------------------------------------------------------------------------

loc_D0B1:	 ; Если символ >= 0x87
		cp 87h ; 'З'
		jp nc, printerEcho_1

printerEcho_spc: ; Заменяем символ на пробел
		ld c, ' '

doPrinterEcho:	 ; Если это удаление, выходим
		ld a, c
		cp 8
		ret z
		; Перевод курсора в начало экрана - выходим
		cp 0Ch
		ret z
		; Если <= 0x18
		cp 18h
		jp c, printerEcho_crLf
		; Если <= 0x1C выходим
		cp 1Ch
		ret c
		; Если очистка экрана - выходим
		cp 1Fh
		ret z
		; Остальные служебные коды заменяем на пробел
		jp c, printerEcho_spc
		; Если <= 0x60, печатаем
		cp 60h
		jp c, printer
		; ?
		ld hl, (v_koi8)
		cp 7Fh
		jp nc, loc_D0B1 ;+
		inc l
		jp nz, printer
		or 80h
		ld c, a

printerEcho_1:   ; Если символ меньше 0xC0, выходим
		cp 0C0h
		ret c

		; Если v_tapeMode = '0', выводим символ без изменений
		ld a, (v_tapeMode)
		sub '0'
		jp z, printer

		; Если v_tapeMode = '1', выводим с преобразованим
		dec a
		jp nz, printerXlat

		; Иначе перед выводом каждого симовола выводим 27, 82, 1
		; Из документациии к принтерам Epson
		; Выбрать французский набор символов
		ld hl, 1B52h
		ld a, 1
		call	printerEsc

		; Вывод C на принтер
		call	printer		 ; Вывод	байта на принтер

		; Выводим 27, 82, 0
		; Выбрать американский набор символов
		xor a

printerEsc:	push bc
		 ld c, h
		 call	printer		 ; Вывод	байта на принтер
		 ld c, l
		 call	printer		 ; Вывод	байта на принтер
		 ld c, a
		 call	printer		 ; Вывод	байта на принтер
		pop bc
		ret

    ENDIF
