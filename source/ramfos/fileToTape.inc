;+---------------------------------------------------------------------------
; RAMFOS
; Сохранить файл на магнитофон
;
; 2013-11-01 Дизассемблировано vinxru
;----------------------------------------------------------------------------

fileToTape:	 ; Сначала записываем только 15 байт.
		push hl
		 push de
		  ld de, 15
		  call	fileToTape_0
		 pop de
		pop hl

		; Потом тот же блок данных, но целиком

fileToTape_0:	 ; Пилот тон
		call	j_tapeWritePilot

		; Запись длины
		ld c, e
		xor a
		call	j_tapeWrite
		ld c, d
		xor a
		call	j_tapeWrite

		; Укорочение первого импульса магниофона *15 тактов
		ld b, 8
fileToTape_1:	  ; Если записывать нечего, выходим
		 ld a, e
		 or d
		 ret z
		 ; Читаем байт с диска
		 call	pageRead
		 inc hl
		 ; Уменьшаем счетчик
		 dec de
		 ; Записываем на магнитофон
		 ld a, b
		 call	j_tapeWrite
		jp fileToTape_1
