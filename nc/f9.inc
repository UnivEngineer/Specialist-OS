;----------------------------------------------------------------------------
; MXOS NC.COM
; Клавиша F9 - Сохранение файла на магнитофон
;
; 2013-12-18 Дизассемблировано vinxru
;----------------------------------------------------------------------------

f9:		; Рисуем окно
		call	drawWindowIfSel

		; Выводим SAVING FROM A: TO TAPE
        ; центр экрана это x = 60h
        ; половина ширина строки w = 20*3/2 = 30
        ; координата строки x = 60h-30 = 42h
		lxi	h, 425Eh
		shld	v_cursorY
		lxi	h, aSaveFromToTape
		call	printString
		call	printSelDrive

		; ?
		call	printInvSelFile

		; Ждем нажатия Enter или ESC.
loc_D39C:	call	keyScan
		cpi	1Bh
		jz	mainReload
		cpi	0Dh
		jnz	loc_D39C

		; Сохранение SP	и установка обр	ошибок.
		call	prepareTapeLoad

		call	drawWindowIfSel

		; Вывод SAVING TO TAPE
        ; центр экрана это x = 60h
        ; половина ширина строки w = 14*3/2 = 21
        ; координата строки x = 60h-21 = 4Bh
		lxi	h, 4B5Eh
		shld	v_cursorY
		lxi	h, aSavingToTape	
		call	printString

		; Загрузка файла
		call	loadSelFileAt0

		; Получение адреса загрузки и длины
		push	d
		push	h
		lxi	d, 0Ch
		dad	d
		mov	e, m
		inx	h
		mov	d, m
		pop	h
		push	d
		push	h
		lxi	h, 0
		dad	d
		xchg
		lxi	h, 0

		; Расчет контрольной суммы
		call	calcCS
		mov	h, b
		mov	l, c
		shld	v_tapeSaveCRC

		; Пилот-тон
		call	tapeWritePilot

		; Начало файла
		mvi	a, 0D9h
		call	tapeWrite
		call	tapeWrite
		call	tapeWrite

		pop	h ; откуда
		lxi	b, 8 ; длина
		call	tapeWriteBlock

		; Пилот-тон
		call	tapeWritePilot

		pop	b ; Длина
		pop	h ; Начальный адрес
		call	tapeWriteWord

		dad	b ; Конечный адрес		
		call	tapeWriteWord

		; Запись файла
		lxi	h, 0
		call	tapeWriteBlock	; hl-адрес, bc-длина-1

		; Запись контрольной суммы
		lhld	v_tapeSaveCRC
		call	tapeWriteWord

		jmp	mainReload

