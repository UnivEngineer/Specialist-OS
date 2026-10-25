;+---------------------------------------------------------------------------
; RAMFOS
; Преобразвоние строки в 16-ричное число
;
; На входе
;  de - Строка
;
; На выходе
;  bc       - Сохраняется
;  de       - Следующий символ за запятой или концом строки
;  jz       - Ошибка
;  jnz, jc  - Конец строки
;  jnz, jnc - Пробел или запятая
;  hl       - Число
;
; 2013-11-01 Дизассемблировано vinxru
;----------------------------------------------------------------------------

strToHex:
    IF SIZE_OPTIMIZATION == 0
		push bc
		 call	strToHex2
		pop bc
		ret
    ENDIF

; ---------------------------------------------------------------------------

strToHex2:	ld hl, 0
    IF SIZE_OPTIMIZATION == 0
		ld b, 0
    ENDIF

strToHex2_loop:	 ; Чтение символа
		ld a, (de)
		inc de

		; Если конец строки, выходим с флагами NZ, C
		cp 0Dh
		jp z, ora_a_stc_ret

		; Если пробел или запятая, выходим с флагами NZ, NC
		cp ','
		jp z, ora_a_ret
		cp ' '
		jp z, ora_a_ret

		; Если некорректный символ, выходим с флагами С,Z
		sub '0'
		jp m, xra_a_stc_ret

		; Если это цифра
		cp 10
		jp m, strToHex2_0

		; Если некорректный символ, выходим с флагами С,Z
		cp 11h
		jp m, xra_a_stc_ret
		cp 17h
		jp p, xra_a_stc_ret

		; Это буквы A..F
		sub 'A'-'0'-10

strToHex2_0:	
    IF SIZE_OPTIMIZATION == 0
		ld c, a
    ENDIF
		; hl *=	16
		add hl, hl
		add hl, hl
		add hl, hl
		add hl, hl

		; Если переполнение, то	выйти c флагами С,Z
		jp c, xra_a_stc_ret
		
		; hl += bc
    IF SIZE_OPTIMIZATION
		push bc
		ld b, 0
		ld c, a
    ENDIF
		add hl, bc
    IF SIZE_OPTIMIZATION
		pop bc
    ENDIF
		jp strToHex2_loop
