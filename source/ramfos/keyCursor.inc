;+---------------------------------------------------------------------------
; RAMFOS
; Ожидание нажатия клавиши с курсором
;
; 2013-11-01 Дизассемблировано vinxru
;----------------------------------------------------------------------------

keyCursor:	 ; Рисуем курсор XOR-ом
		call	keyCursor1
		; Стираем курсор XOR-ом
		call	keyCursor1
		; Если была нажата клавиаша, выходим
		ret nz
		; Бесконечное ожидание
		jp keyCursor

;----------------------------------------------------------------------------

keyCursor1:	
    IF FAST_PRINT != 1
		; Код символа курсора
		ld c, 17h

		; Положение курсора в пикселях 		
		ld hl, (v_cursorPos)
		ex de, hl

		; Получение изображения символа hl и адреса в видеопамяти de
		call	calcCharAddr2

		; Корректировка адреса
		ld a, e
		sub 7
		ld e, a

		; Рисование курсора
		call	drawChar
    ELSE  ;----------------------------------------------------------------------
		; Положение курсора в пикселях 		
		ld hl, (v_cursorPos)
		ex de, hl

		; Получение адреса в видеопамяти de
		call calcCharAddr

		; Исходный символ
		ld hl, 3Fh

		; Сдвигаем
		ld a, c
drawCursor_0:	add hl, hl
		 add hl, hl
		 dec a
		jp nz, drawCursor_0

		; Цикл b
		ld b, 8
drawCursor_1:	  ; *de++ ^= hl
		 ld a, (de)
		 xor h
		 ld (de), a
		 inc d
		 ld a, (de)
		 xor l
		 ld (de), a
		 dec d
		 dec e
		 ; Цикл
		 dec b
		jp nz, drawCursor_1
    ENDIF  ;---------------------------------------------------------------------
		; Ждем нажатия клавиши
		ld hl, CURSOR_BLINK_SPEED
keyCursor1_0:	 call	keyCheck
		 ret nz
		 dec hl
		 ld a, h
		 or l
		jp nz, keyCursor1_0
		ret
