;+---------------------------------------------------------------------------
; RAMFOS
; Вывод на экран нескольких символов
;
; На входе:
;  b - кол-во
;  с - символ
;
; 2013-11-01 Дизассемблировано vinxru
;----------------------------------------------------------------------------

    IF ALL_CHARS
                 
printChars:	push bc
		  call	printCharAll
		 pop bc
		 dec b
		jp nz, printChars
		ret

    ELSE

printChars:	 call	j_printChar
		 dec b
		 ret z
		jp printChars

    ENDIF
