;+---------------------------------------------------------------------------
; RAMFOS
; Стандартная точка входа C842
; Печать 16-ричного числа на принтере
;
; На входе
;  A - число
;
; Все регистры сохраняются
;
; 2013-11-01 Дизассемблировано vinxru
;----------------------------------------------------------------------------

    IF NO_PRINTER == 0

printerHex:	push af
		 rrca
		 rrca
		 rrca
		 rrca
		 call	printerHex_1
		pop af
printerHex_1:	push af
		 and 0Fh
		 cp 10
		 jp m, printerHex_2
		  add a, 'A'-'0'-10
printerHex_2:	add a, '0'
		 push bc
		  ld c, a
		  call	printer
		 pop bc
		pop af
		ret

    ENDIF
