;----------------------------------------------------------------------------
; RAMFOS
; Вывод символа на экран
;
; На входе:
;  с - символ
;
; 2013-11-01 Дизассемблировано vinxru
;----------------------------------------------------------------------------

printChar:	push hl
		push de
		push bc
		push af
		 call	printChar_int
		pop af
popa_ret:	pop bc
		pop de
		pop hl
		ret

; ---------------------------------------------------------------------------

printChar_int:	 ; Обработка ESC-последовательности
		ld a, (v_escMode)
		or a
		jp nz, printChar_esc

		; de - положение курсора в пикселях
		ld hl, (v_cursorPos)
		ex de, hl

		; Эхопечать и обрабатываем нажатые клавиши во время вывода
		push bc
		call	keyScanWhilePrint
		pop bc

		; Символ
		ld a, c

		; hl - положение курсора в пикселях
		ex de, hl

		; Спец символы
		cp 1Fh
		jp z, clearScreen
		cp 18h
		jp z, cursorRight
		cp 8
		jp z, cursorLeft
		cp 19h
		jp z, cursorUp
		cp 1Ah
		jp z, cursorDown
		cp 0Ch
		jp z, loc_CE4B
		cp 0Ah
		jp z, cursorCrLf
		cp 1Bh
		jp z, setEscMode1
		cp 7
		jp z, keySound
		cp 0Dh
		jp z, saveCursorPosX0

		; Символы меньше 0x80 
		cp 80h
		jp c, printChar_high

		; Промежуток C0-FF преобразовать в 80-BF
		sub 40h
		jp m, printChar_1
		sub 3Fh

		; Теперь в A=1-40

		; if(a >= 8) goto loc_CC87;
		cp 8
		jp nc, printChar2
		
printChar_high:	 ; Если символы меньше 0x60
		cp 60h
		jp c, printChar_1

		; Преобразуем KOI-7 в KOI-8
		ld a, (v_koi8)
		or a
		ld a, c
		jp z, printChar_1
		 add a, 40h
printChar_1:	ld c, a

printChar_0:	ld a, h
		cp 0BEh ; '-'

		push bc
		call nc, cursorCrLf
		pop bc

		ld a, h
		add a, 3
		ld (v_cursorPos+1), a ; Положение курсора в пикселях
		ex de, hl

		call	calcCharAddr2	 ; Возможно адрес символа

		push hl
		 ; Исходный символ
		 ld hl, 3Fh
		 ; hl <<= c*2
		 ld a, c
loc_CC09:	add hl, hl
		  add hl, hl
		  dec a
		 jp nz, loc_CC09
		 ; Вывод на экран
		 push bc
		  ld b, 8
		  call	clearChar
		 pop bc
		pop hl

		; if(a == 0x7F) return;
		ld a, b
		cp 7Fh
		ret z

		; if(a == 0x20) return;
		cp 20h
		ret z

		; e++;
		inc e

		; Продолжение на drawChar
; Вывод	символа
; hl - символ
; de - адрес в видеопамяти
; с  - смещение в двойных пикселях

drawChar:	 ; Высота символа
		ld b, 8
drawChar_0:	push hl

		; Очередной байт из HL в HL
		ld l, (hl)
		ld h, 0

		; Сдвигаем HL на C*2 бит
		ld a, c
drawChar_1:	add hl, hl
		add hl, hl
		dec a
		jp nz, drawChar_1

		; Накладываем символ (*de++ ^= hl)
		ld a, (de)
		xor h
		ld (de), a
		inc d
		ld a, (de)
		xor l
		ld (de), a
		dec d
		inc e

                ; hl++
		pop hl
		inc hl

		; Конец цикла
		dec b
		jp nz, drawChar_0

		ret

; ---------------------------------------------------------------------------
; Очистка знакоместа.
;
; На входе
;  de - адрес в видеопамяти
;  hl - символ 
;  b - высота
; ---------------------------------------------------------------------------

clearChar:	 ; Если включено инвертирование, то заполняем знакоместо белым
		ld a, (v_inverse)
		or a
		jp nz, clearChar_1

		; Инвертируем HL
		ld a, h
		cpl
		ld h, a
		ld a, l
		cpl
		ld l, a

		; Цикл b
clearChar_0:	  ; *de++ &= hl
		 ld a, (de)
		 and h
		 ld (de), a
		 inc d
		 ld a, (de)
		 and l
		 ld (de), a
		 dec d
		 dec e
		 ; Цикл
		 dec b
		jp nz, clearChar_0
		ret

; ---------------------------------------------------------------------------

clearChar_1:	  ; *de++ |= hl
		 ld a, (de)
		 or h
		 ld (de), a
		 inc d
		 ld a, (de)
		 or l
		 ld (de), a
		 dec d
		 dec e
		 ; Цикл
		 dec b
		jp nz, clearChar_1
		ret
