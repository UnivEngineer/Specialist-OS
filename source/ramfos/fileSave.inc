;+---------------------------------------------------------------------------
; RAMFOS
; Сохранить файл
;
; 2013-11-01 Дизассемблировано vinxru
;----------------------------------------------------------------------------

fileSave:	 ; Найти свободную страницу и рассчитать контрольную сумму
		call	fileFindFree
		ret c

		; Общая функция
		jp fileSaveCom

;----------------------------------------------------------------------------
; RAMFOS
; Сохранить файл с вводом имени с клавиатуры
;
; 2013-11-01 Дизассемблировано vinxru
; ---------------------------------------------------------------------------

fileSaveDlg:	 ; Вывод текста "Сохранить?"
		call	printStringEx
		DB C_INVERSE, C_BOTTOMLINE, ' ', 0F3h, 0CFh, 0C8h, 0D2h, 0C1h, 0CEh, 0C9h, 0D4h, 0D8h, 3Fh, C_NORMAL, 0

		; Ввод имени файла
		call	j_inputFileName
		ret z

		; Поиск подходящей страницы
		call	fileFindFree

		; В случае ошибки вывести "мал диск" и ждать нажания ESC
		jp c, outOfSpace

fileSaveCom:	 ; Запись заголовка файла на диск
		ld bc, v_header2
		call	fileSaveHeaderAny

		; BC = начало файла, DE = длина файла - 1
		push hl
		 call	fileGetStartAndSize2
		 ex de, hl
		 ld c, l
		 ld b, h
		pop hl

		; Запись тела файла (запись на диск по адресу HL из памяти BC длиной DE+1)
		call	pageWriteBlock 

		; Продолжение в fileChanged		
