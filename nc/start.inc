;----------------------------------------------------------------------------
; MXOS NC.COM
; Запуск
;
; 2013-12-18 Дизассемблировано vinxru
;----------------------------------------------------------------------------

Start:		; Отключаем инверсию
		lxi	h, 0
		shld	v_inverse

		; *** Однократно вызываем FORMAT ***

		; Устанавливаем свой драйвер
		call	swapDriver

		; Если в рам-диске по адресу 0FF00h (последний неполный сектор)
        ; есть специальная метка, значит уже запускали
		lxi h, 0FF00h
		call	0FFC0h ; читаем 1 байт из рам-диска по адресу hl
		cpi	5Ah
		jz	formatCalled

		; Восстанавливаем драйвер
		call	swapDriver

		; Сохраняем состояние
		call	saveState

		; В оригинале: запуск "A:FORMAT.COM B:"
		; SpaceEngineer: вместо этого запускаем "A:FORMAT.BAT"
		lxi	h, aFormatB
		jmp	fileExec

; ---------------------------------------------------------------------------

formatCalled:	; Копируем состояние из ДОЗУ в ОЗУ
		inx	h
		lxi	d, activePanel
		mvi	c, 9    ; 9 байт
loc_D026:
        call	0FFC0h
		stax	d
		inx	h
		inx	d
		dcr	c
		jnz	loc_D026

		; Восстанавливаем драйвер
		call	swapDriver

        ; Очистка экрана
		mvi	c, 1Fh

		; Вывод	подсказки
		COLOR(COLOR_HELP_F)
		call	printChar
		lxi	h, 1FFh
		shld	v_cursorY
		lxi	h, aF1LeftF2RighF3		
		call	printString

		; Раскрашиваем подсказку
		COLOR(COLOR_HELP_TEXT)
		lxi	h, 92F7h
#if SHOW_F9
        mvi	d, 9
#else
        mvi	d, 8
#endif
loc_D049:
#if SHOW_F9
		mvi	b, 3
#else
        mvi	b, 4
#endif
		call	inverseRect	; hl - адрес, b	- ширина, высота 9
		inr	h
		inr	h
		dcr	d
		jnz	loc_D049

		; Очищаем ком строку
		call	clearCmdLine

