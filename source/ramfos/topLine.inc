;+---------------------------------------------------------------------------
; RAMFOS
; Вывод идникаторов в верхнюю строку
; Эта функция вызывается только из printIndicators и clearTopLine
;
; 2013-11-01 Дизассемблировано и доработано vinxru
;----------------------------------------------------------------------------

    IF FAST_PRINT
INDCATORS_SEP = ' '
    ELSE
INDCATORS_SEP = 11h
    ENDIF

printIndicatorsInt:	
		; Если дата хранится в ОЗУ, проверяется, введена ли дата
    IF DATE_IN_RAM
		ld a, (v_header2_saved)
		cp 8Bh
		call nz, inputDate
    ENDIF
		; Установка положения курсора (0 стока 45 столбец)
		ld hl, 8708h
		ld (v_cursorPos), hl

		; Если дата хранится в ДОЗУ, проверяется, введена ли дата
    IF DATE_IN_RAM == 0
		; Читаем из нулевой страницы байт по адресу v_aramPageEnd+1
		ld hl, (v_aramPageEnd)
		inc hl
		xor a
		call	j_pageReadAny
		
		; Если там записан не 8B, то переходим к вводу даты
		ld a, c
		cp 8Bh
		push hl
		jp nz, inputDate
    ENDIF

		; Разделитель
		ld a, INDCATORS_SEP
		call	printCharA

		; Выводим скорость записи на ленту
		ld a, (v_tapeMode)
		call	printCharA

		; Выводим режим эхопечати
		ld a, (v_printerEcho)
		call	printCharA

		; Выводим CAPS LOCK
		ld hl, (v_rusLat)
		dec h
		ld a, 0Bh
		jp m, loc_C9D5
		 ld a, 0Fh
loc_C9D5:	call	printCharA

		; Выводим РУС/LAT
		inc l
		ld hl, a_lat
		jp nz, loc_C9E2
		  ld hl, a_rus
loc_C9E2:	call	printString

		; Выводим KOI8/7
		ld a, (v_koi8)
		or a
		ld a, '8'
		jp z, loc_C9EF
		  dec a
loc_C9EF:	call	printCharA

    IF FAST_PRINT
		ld a, ' '
		call	printCharA
    ELSE
		call	printChar6
    ENDIF

    IF DATE_IN_RAM
		; Разделитель
		ld a, INDCATORS_SEP
		call	printCharA

		; Вывод на экран даты
		ld a, (v_header2_date)
		call	printHex

    IF NICE
		ld a, '-'
		call	printCharA
    ENDIF
		; Вывод на экран даты
		ld a, (v_header2_date+1)
		call	printHex
    IF NICE
		ld a, '-'
		call	printCharA
    ENDIF
		; Вывод на экран даты
		ld a, (v_header2_date+2)
		call	printHex
    ELSE
		pop hl

		; Адрес для сохранения даты
		ld de, v_header2_saved

		; Сохраняем C в v_header2_saved <--- ????
		ld a, c
		ld (de), a

		; Разделитель
		ld a, INDCATORS_SEP
		call	printCharA

		; Вывод на экран даты
		ld b, 3 ; 3 байта
loc_CA02:	inc hl
		 inc de
		 ; Читаем байт из нулевой страницы
		 xor a
		 call	j_pageReadAny
		 ; Запись в памяти
		 ld a, c
		 ld (de), a
		 ; Вывод на экран
		 call	printHex
		 ; Цикл 3-1
		 dec b
		jp nz, loc_CA02
    ENDIF

    IF FAST_PRINT
		ret
    ELSE
		jp printChar6
    ENDIF

;----------------------------------------------------------------------------
; RAMFOS
; Лишний код
;
; 2013-11-01 Дизассемблировано vinxru
;----------------------------------------------------------------------------

    IF NO_PRINT_STATUS_BUG == 0
printIndicatorsEnd:
		pop af
		call nz, changeInverse
    ENDIF

;----------------------------------------------------------------------------
; RAMFOS
; Очистка верхней строки и перемещение туда курсора
; Вызывается при печати символа 08Fh
;
; 2013-11-01 Дизассемблировано и доработано vinxru
;----------------------------------------------------------------------------

clearTopLine:	
    IF COLOR_SUPPORT
		; Цвет по умолчанию
		ld a, INIT_SCREEN_COLOR
		call	setColor
    ENDIF

		; Реальная очистка верхней строки
		call	clearTopLine2

		; Устанавливаем курсор в верхнюю строку
		ld hl, 8
		ld (v_cursorPos), hl

		; ?
		ld hl, 140Ah
		ld (v_topLineHeight), hl

;----------------------------------------------------------------------------
; RAMFOS
; Вывод идникаторов в верхнюю строку с сохранением курсора, цвета и инверсии
; Эта функция вызывается только из keyScan
;
; 2013-11-01 Дизассемблировано и доработано vinxru
;----------------------------------------------------------------------------

printIndicators:
		; Не рисовать индикаторы
		ld a, (v_topLineHeight)
		or a
		ret z

		; Запоминаем положение курсора
		ld hl, (v_cursorPos)

		; Запоминаем инверсию в стеке
		ld a, (v_inverse)
		or a
		push af
    IF FAST_PRINT
		; Если инверсия не была включена, включаем её.
		call z, changeInverse
    ELSE
		; Если инверсия была включена, выключаем её.
		call nz, changeInverse
    ENDIF	

		; Сохранение цвета и установка стандартного цвета
    IF COLOR_SUPPORT
		ld a, (v_color)
		push af
		ld a, 0F0h
		call	setColor
    ENDIF

		; Вывод статусной строки
		push hl
		 call	printIndicatorsInt
		pop hl

		; Восстановление цвета
    IF COLOR_SUPPORT
		pop af
		call	setColor
    ENDIF
	
		;  Восстановление положения курсора
		ld (v_cursorPos), hl
		 
    IF NO_PRINT_STATUS_BUG == 0
		jp nz, printIndicatorsEnd
    ENDIF

		;  Восстановление инверсии
		pop af
    IF FAST_PRINT
		ret nz
    ELSE
		ret z
    ENDIF	
		; продолжается в changeInverse
