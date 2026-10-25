;----------------------------------------------------------------------------
; RAMFOS
; Ускоренная функция выводе символа на экран.
;
; На входе:
;  с - символ
;
; 2013-11-01 Разработано vinxru
;----------------------------------------------------------------------------

printChar:	push hl
		push de
		push bc
		push af
printChar_pol:	 call	printChar_int
		pop af
popa_ret:	pop bc
		pop de
		pop hl
		ret

; ---------------------------------------------------------------------------

printCharAll:	ld hl, (v_cursorPos)
		jp printChar_0

; ---------------------------------------------------------------------------

printChar_int:	
    IF NO_PRINTER == 0
		; Если включен эхо режим на принтер
		ld a, (v_printerEcho)
		cp 0D2h
		call z, doPrinterEcho
    ENDIF
		; hl - положение курсора в пикселях		
		ld hl, (v_cursorPos)

		; Служебные символы
		ld a, c ; Символ		
		cp 1Fh
		jp z, clearScreen

		; Если символ >0x1F, пропускаем проверки ниже
		jp nc, fast

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
		jp z, printChar_esc
		cp 7
		jp z, keySound
		cp 0Dh
		jp z, saveCursorPosX0
				
fast:		 ; Если меньше 0x60, переходим к печати
		cp 60h
		jp c, printChar_0

		; Символы меньше 0x80 
		cp 80h
		jp c, printChar_koi

		; Промежуток C0-FF преобразовать в 80-BF
		sub 40h
		jp m, loc_CBF1

		; Промежуток 80-BF преобразовать в 1-40
		sub 3Fh

		; Обрабатываем промежуток 87-BF
		cp 8
		jp nc, printChar2
		ld c, a

		; Промежуток 80-86 преобразовать в символы 1-7
		
loc_CBE2:	 ; Если меньше 0x60, переходим к печати
		cp 60h
		jp c, printChar_0

		; Трансляция из KOI-7 в KOI-8
printChar_koi:	ld a, (v_koi8)
		or a
		jp z, printChar_0
		ld a, c
		add a, 40h

loc_CBF1:	ld c, a

printChar_0:	 ; Если cursorX >= 190, то перевод строки
		ld a, h
		cp 190
		call nc, cursorCrLf

		; Не позволяем снимать инверсию в служебных строках
                ld a, (v_screenHeight)
		cp l
		jp c, printChar_u
                ld a, (v_topLineHeight)
		cp l
		ld a, (v_inverse)
		jp c, printChar_i
printChar_u:	ld a, -1
printChar_i:	ld (v_inverse2), a

		; Увеличиваем положение курсора
		ld a, h
		add a, 3
		ld (v_cursorPos+1), a ; Положение курсора в пикселях

		; Положение курсора в DE
		ex de, hl

		; Расчет адреса символа и знакоместа
		call	calcCharAddr3

		; Косяк в координатах
		ld a, e
		sbc a, 8
		ld e, a

		ld b, 9

		; Дублировать строку?
		xor A
		or (hl)
		ld a, (v_inverse2)
		jp p, drawCharDbl
		 xor (hl)
drawCharDbl:
		 		 
		dec hl
		dec c
		dec c
		dec c
		jp z, printCharS4
		dec c
		jp z, printCharS2
		dec c
		jp z, printCharS0

;----------------------------------------------------------------------------
; Сдвиг на 6

printCharS6:	CALL	printCharS6_0
printCharS6_0:	rra
		 rra
		 rra
		 rra
		 push af
		  and 00000011b
		  ld c, a
		  ld a, (de)
		  and 11111100b
		  xor C
		  ld (de), a
		  inc d
		 pop af
		 rra
		 and 11110000b
		 ld c, a
		 ld a, (de)
		 and 00001111b
		 xor C
		 ld (de), a
		 dec d
		 inc de
		 inc hl
		 ld a, (v_inverse2)
		 xor (hl)
		 dec b
		jp nz, printCharS6_0
		jp drawChar_ll

;----------------------------------------------------------------------------
; Сдвиг на 0

printCharS0:    CALL	printCharS0_0
printCharS0_0:
		 add a, A
		 add a, A
		 ld c, a
		 ld a, (de)
		 and 00000011b
		 xor C
		 ld (de), a
		 inc de
		 inc hl
		 ld a, (v_inverse2)
		 xor (hl)
		 dec b
		jp nz, printCharS0_0
		jp drawChar_ll
	
;----------------------------------------------------------------------------
; Сдвиг на 2

printCharS2:    CALL	printCharS2_0
printCharS2_0:	and 03Fh
		 ld c, a
		 ld a, (de)
		 and 11000000b
		 xor C
		 ld (de), a
		 inc de
		 inc hl
		 ld a, (v_inverse2)
		 xor (hl)
		 dec b
		jp nz, printCharS2_0
		jp drawChar_ll

;----------------------------------------------------------------------------
; Сдвиг на 4

printCharS4:	CALL	printCharS4_0
printCharS4_0:	rra
		 rra
		 push af
		  and 00001111b
		  ld c, a
		  ld a, (de)
		  and 11110000b
		  xor C
		  ld (de), a
		  inc d
		 pop af
		 rra
		 and 011000000b
		 ld c, a
		 ld a, (de)
		 and 000111111b
		 xor C
		 ld (de), a
		 dec d
		 inc de
		 inc hl
		 ld a, (v_inverse2)
		 xor (hl)
		 dec b
		jp nz, printCharS4_0

;----------------------------------------------------------------------------

drawChar_ll:	dec hl
		xor A
		or (hl)
		ld a, (v_inverse2)
		jp p, drawCharDbl2
		 xor (hl)
drawCharDbl2:	inc b
		RET

; ---------------------------------------------------------------------------

printChar_esc:	ld hl, printChar_esc1
		jp setEscMode
		
; ---------------------------------------------------------------------------

printChar_esc1:	 ; Обработка первого символа ESC-последовательности

		; Если символ Y, перейти к обработке второго символа
		ld a, c
		cp 'Y'
		ld hl, printChar_esc2
		jp z, setEscMode

		ld hl, (v_cursorPos)
		call	printChar_0

setEscMode0:	ld hl, printChar_int
setEscMode:	ld (printChar_pol+1), hl
		ret

; ---------------------------------------------------------------------------

printChar_esc2:	 ; Вычесть из буквы 20h
		ld a, c
		sub 20h
		ld (v_escRow), a
		ld hl, printChar_esc3
		jp setEscMode

; ---------------------------------------------------------------------------

printChar_esc3:	 ; Вычесть из буквы 20h
		ld a, c
		sub 20h
		ld l, a
		ld a, (v_escRow)
		ld h, a
		call	setCursorPos
		jp setEscMode0
		
v_inverse2:	DB 0
