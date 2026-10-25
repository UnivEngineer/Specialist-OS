;+---------------------------------------------------------------------------
; RAMFOS
; Стандартная точка входа C845
; Ввовод имени файла с клавиатуры в v_header2_name
;
; 2013-11-01 vinxru
;----------------------------------------------------------------------------

inputFileName:	push hl
		push de
		push bc

		; Промежуточный адрес возврата
		ld hl, popa_ret
		push hl

 		; Выводим сообщение на экран и сохраним курсор строки
		ld hl, a_inputFileName
		call	printString

		; Удалим из имени файла все левые символы
		call	printFileName2

loc_C873:	 ; Восстановить курсор строки
		call	popCursorLine

		; Ввод имени файла
		ld hl, v_header2_name
		ld de, 0CFFh
		call	input

		; Нажат ESC
		ret z

		; Нажата левая клавиша, вывести знак вопроса и вернуться к вводу
		ld hl, loc_C873
		push hl
		jp nc, xra_a_stc_ret

		; Сдвинуть курсор
		ld hl, (v_pushCursorLn)
		ld a, h
		add a, 3*8
		ld h, a
		ld (v_cursorPos), hl

		; Разделитель между именем и расширением должен быть пробелом
		ld hl, v_header2_ext-1
		ld b, 4
		ld a, (hl)
		cp 20h
		jp nz, xra_a_stc_ret

		; Расширение
loc_C89C:	dec b
		 call	printCharA
		 jp z, loc_CE8A ; Переход на pop h, ora a, ret
		 inc hl
		 ld a, (hl)
		 cp 20h
		jp nz, loc_C89C
		
xra_a_stc_ret:	 ; Вывод знака вопроса
		ld a, '?'
		call	printCharA
		xor a

ora_a_stc_ret:	or a
		scf
		ret
