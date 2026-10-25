;----------------------------------------------------------------------------
; RAMFOS
; Вывод символа на экран. Продолжение printChar.inc
;
; 2013-11-01 Дизассемблировано vinxru
;----------------------------------------------------------------------------

printChar2:	sub 21h

printChar_esc1:	 ; Обработка первого символа ESC-последовательности

		; Если символ Y, перейти к обработке второго символа
		ld a, c
		cp 'Y'
		ld a, 2
		jp z, setEscMode

		; Доработка для вывода всех символов знакогенератора
    IF ALL_CHARS
printCharAll:
		xor a
		ld (v_escMode), a
		ld hl, (v_cursorPos)
		jp printChar_0
    ENDIF

; ---------------------------------------------------------------------------

setEscMode0:	xor a
setEscMode:	ld (v_escMode), a
		ret

; ---------------------------------------------------------------------------

printChar_esc2:	ld (v_escRow), a
		ld a, 3
		jp setEscMode

; ---------------------------------------------------------------------------

setEscMode1:	ld a, 1
		jp setEscMode
		
; ---------------------------------------------------------------------------

printChar_esc:	 ; Обработка ESC последовательности

		; Если printChar_esc1Mode=1
		dec a
		jp z, printChar_esc1
		ld b, a

		; Вычесть из буквы 20h
		ld a, c
		sub 20h

		; Если printChar_esc1Mode=2, сохранить 
		dec b
		jp z, printChar_esc2

		ld l, a
		ld a, (v_escRow)
		ld h, a
		call	setEscMode0

		; Продолжение setCursorPos
