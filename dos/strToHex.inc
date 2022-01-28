;+---------------------------------------------------------------------------
; MXOS
; Преобразвоние строки в 16-ричное число
; Заимствовано из RAMFOS
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
        lxi	h, 0		

strToHex_loop:	; Чтение символа
		ldax	d
		inx	d

		; Если конец строки, выходим с флагами NZ, C
		cpi	0Dh
		jz	ora_a_stc_ret

		; Если пробел или запятая, выходим с флагами NZ, NC
		cpi	','
		jz	ora_a_ret		
		cpi	' '
		jz	ora_a_ret

		; Если некорректный символ, выходим с флагами С,Z
		sui	'0'
		jm	xra_a_stc_ret		

		; Если это цифра
		cpi	10
		jm	strToHex_0

		; Если некорректный символ, выходим с флагами С,Z
		cpi	11h
		jm	xra_a_stc_ret
		cpi	17h
		jp	xra_a_stc_ret

		; Это буквы A..F
		sui	'A'-'0'-10

strToHex_0:	
		; hl *=	16
		dad	h
		dad	h
		dad	h
		dad	h

		; Если переполнение, выходим c флагами С,Z
		jc	xra_a_stc_ret
		
		; hl += bc
		push b
		mvi  b, 0
		mov  c, a
		dad  b
		pop  b

		jmp	strToHex_loop

xra_a_stc_ret:	; Вывод знака вопроса
        mvi	c, '?'
        call	printChar
        xra	a

ora_a_stc_ret:
        ora	a
        stc
        ret

ora_a_ret:
        ora	a
        ret
