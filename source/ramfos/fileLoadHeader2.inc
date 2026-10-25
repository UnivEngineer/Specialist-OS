;+---------------------------------------------------------------------------
; RAMFOS
; Расчитать доп переменные загруженного заголовка файла
;
; На входе
;   v_curFile      - Адрес текущего файла в ДОЗУ
;   f_header       - Заголовок файла
;
; На выходе
;   f_fileBodySize - Длина файла без заголовка - 1
;   f_fileSize     - Длина файла с заголовком
;   f_nextFile     - Адрес следующего файла в ДОЗУ
;
;   Если не включено в ДОЗУ по адресу v_curFile + f_fileSize 
;   записывается v_curFile
;
; 2013-11-01 Дизассемблировано vinxru
;----------------------------------------------------------------------------

fileLoadHeader2: ; Длина файла без заголовка
		; f_fileBodySize = v_header_stop - v_header_start
		call	loadDStartDStop	
		ex de, hl
		call	sub_hl_de
		ld (f_fileBodySize), hl

		; Длина файла с заголовом
		; f_fileSize = f_fileBodySize+1
		ld de, FILE_HEADER_SIZE+1
		add hl, de
		ld (f_fileSize), hl

		; Конец файла
		; de = v_curFile + f_fileSize
		ex de, hl
		ld hl, (v_curFile)
		ex de, hl
		add hl, de

		; Обновляем в ДОЗУ указатель на наш файл, который находится
		; перед следующим файлом.
		ld c, e
		call	pageWrite
		inc hl
		ld c, d
		call	pageWrite
		inc hl

		; Адрес следующего файла
		; f_nextFile =  v_curFile + f_fileSize + 2
		ld (f_nextFile), hl

		; Возвращаем ноль и Z
		xor a
		ret
