;+---------------------------------------------------------------------------
; RAMFOS
; Стандартная точка входа C815
; Вывод 16-ричного числа на экране
;
; На входе
;  A - число
;
; 2013-11-01 Дизассемблировано vinxru
;----------------------------------------------------------------------------

printHex:	push af
		rrca
		rrca
		rrca
		rrca
		call	printHex_1
		pop af
printHex_1:	push af
		and 0Fh
		cp 10
		jp m, printHex_2
		 add a, 7
printHex_2:	add a, '0'
printCharA_popret:
		call	printCharA
pop_psw_ret:	pop af
		ret
