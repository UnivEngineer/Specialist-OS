;+---------------------------------------------------------------------------
; RAMFOS
; Вывести имя файла на экран и удалить из него все недопустимые символы
;
; 2013-11-01 Дизассемблировано vinxru
;----------------------------------------------------------------------------

printFileName:	
		; Разделитель
    IF NICE
		ld c, ' '
    ELSE
		; Разделитель
		ld c, 11h
    ENDIF
		call	printChar

printFileName2:	ld hl, v_header2_name ; Имя
		ld b, 12 ; Длина имени
printFileNam_1:	ld a, (hl)
		 ; Символы меньше 0x20 заменяем на пробел
		 cp 20h
		 jp c, printFileNam_2
		 ; Символы от 0x20 до 0x7E допускаются
		 cp 7Fh
		 jp c, printFileNam_3
		 ; Символы от 0xC0 до 0xFF допускаются
		 cp 0C0h
		 jp nc, printFileNam_3
printFileNam_2:	  ; Заменяем недопустимый символ на пробел
		 ld (hl), 20h
		 ld a, (hl)
printFileNam_3:	  ; Выводим символ на экран
		 call	printCharA
		 ; Следующий символ
		 inc hl
		 dec b
		jp nz, printFileNam_1

		; Разделитель
    IF NICE
		ld c, ' '
		jp printChar
    ENDIF
		; Переход на printChar6
