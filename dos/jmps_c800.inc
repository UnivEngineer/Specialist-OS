j_reboot3:		jmp	reboot3			; C800: Запустить NC.COM
j_getch:		jmp	getch			; C803: Ожидание ввода с клавиатуры
j_tapeRead:		jmp	tapeRead		; C806: Чтение байта с магнитофона
j_printChar:	jmp	printChar		; C809: Вывод символа на экран
j_tapeWrite:	jmp	tapeWrite		; C80C: Запись байта на магнитофон
				jmp	input			; C80F: Ввод строки с клавиатуры
				jmp	j_keyScan		; C812: Получить код нажатой клавиши
				jmp	printHexByte	; C815: Вывести 16-ричное число (байт)
j_printString:	jmp	printString1	; C818: Вывести строку на экран
				jmp	j_keyScan		; C81B: Получить код нажатой клавиши
				jmp	getCursorPos	; C81E: Получить координаты курсора в HL (координаты в пикселях)
				jmp	setCursorPos	; C821: Установить координаты курсора из HL (координаты в пикселях)
				jmp	tapeLoad		; C824: Загрузить программу с магнитофона
				jmp	tapeSave		; C827: Сохранить программу на магнитофон
				jmp	calcCS			; C81A: Расчет контрольной суммы
				jmp	printHexWord	; C82D: Вывести 16-ричное число (слово)
				jmp	getMemTop		; C830: Получить объем доступной памяти
				jmp	setMemTop		; C833: Установить объем доступной памяти
				jmp	printer			; C836: Напечатать байт на принтере
#if RAMFOS_COMPATIBILITY
				jmp	strToHex        ; C839: Преобразвоние строки в HEX формате в число
#else
				jmp	j_reboot3		; C839: Запустить NC.COM
#endif
				jmp	reboot3			; C83C: Запустить NC.COM
				jmp	fileList		; C83F: Получить список файлов
				jmp	fileGetSetDrive	; C842: Получить/установить активное устройство
				jmp	fileCreate		; C845: Создать файл
				jmp	fileLoad		; C848: Загрузить файл по адресу из заголовка этого файла
				jmp	fileDelete		; C84B: Удалить файл
				jmp	fileRename		; C84E: Переименовать файл
				jmp	fileLoadInfo	; C851: Загрузить информацию о файле
				jmp	fileGetSetAddr	; C854: Получить/установить адрес загрузки файла
				jmp	fileGetSetAttr	; C857: Получить/установить атрибуты файла
				jmp	fileNamePrepare	; C85A: Преобразовать имя файла во внутренний формат
				jmp	fileExec		; C85D: Запустить файл
				jmp	installDriver	; C860: Установить драйвер накопителя
j_diskDriver:	jmp	diskDriver		; C863: Драйвер выбранного диска
				jmp	fileLoad2		; C866: Загрузить файл по адресу DE
				jmp	printChar_real	; C869: Вывод символа на экран (только реальные символы)

