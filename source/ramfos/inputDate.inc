;+---------------------------------------------------------------------------
; RAMFOS
; Ввод текущей даты
;
; 2013-11-01 Дизассемблировано vinxru
;----------------------------------------------------------------------------

inputDate:	 ; Вывод приглашения "V3.1/дата:" или "дата:"
		ld hl, a_inputDate
		call	printString
	
		; Ввод даты
		ld hl, buffer
		ld de, 9FFh ; Максимум 9 символов
		call	input

		; Если была ошибка
		jp nc, inputDate
		ld (hl), a

		; Преобразование введенной строки и числа
		call m, parseDate

		; Если преобразование не удалось вводим повторно
		jp z, inputDate

    IF DATE_IN_RAM
		; Сохраняем признак того, что дата введена
		ld a, 8Bh
		ld (v_header2_saved), a

		ret
    ELSE
		; Извлекаем из стека адрес на диске
		pop hl

		; Сохраняем 08Bh
		ld c, 8Bh
		call	inputDate_1
		
		; Сохраняем день на диск
parseDate_r1:	ld c, 22h
		call	inputDate_1

		; Сохраняем месяц на диск
parseDate_r2:	ld c, 09h
		call	inputDate_1

		; Сохраняем год на диск
parseDate_r3:	ld c, 92h
inputDate_1:	xor a
		call	j_pageWriteAny
		inc hl
		inc a
		ret
    ENDIF

    IF NICE
    MACRO A_INPUTDATE
a_inputDate: DB 0Dh, 0C4h, 0C1h, 0D4h, 0C1h, ':', 0 
    ENDM ; "дата:"
    ELSE
    MACRO A_INPUTDATE
a_inputDate: DB 0Dh, 'V', '3', '.', '1', '/', 0C4h, 0C1h, 0D4h, 0C1h, ':', 0 
    ENDM ; "V3.1/дата:"
    ENDIF
