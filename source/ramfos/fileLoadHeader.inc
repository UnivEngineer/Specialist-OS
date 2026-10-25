;----------------------------------------------------------------------------
; RAMFOS
; Загрузить заголовок файла
;
; На входе
;   v_page          - Номер текущей страницы
;   hl              - Адрес текущего файла в ДОЗУ
;
; На выходе, если файл найден
;   b               - Текущая страница ДОЗУ
;   v_curFile       - Адрес текущего файла в ДОЗУ
;   v_header        - Заголовок файла
;   f_fileBodySize  - Длина файла без заголовка - 1
;   f_fileSize      - Длина файла с заголовком
;   f_nextFile      - Адрес следующего файла в ДОЗУ
;
;   Если не включено в ДОЗУ по адресу v_curFile + f_fileSize 
;   записывается v_curFile
;
; На выходе, если файл не найден
;   v_pageFreePtr, v_curFile - Адрес первого свободного байта
;   v_pageFreeTotal      - Свободно байт
;
; 2013-11-01 Дизассемблировано vinxru
;----------------------------------------------------------------------------

fileLoadHeader:	 ; Сохраняем смещение текущего файла в v_curFile
		ld (v_curFile), hl

		; Читаем заголовок файла с диска в v_header
		ld de, v_header
		push de
		 ld bc, FILE_HEADER_SIZE-1
		 call	pageReadBlock
		pop de

		; Помещаем в В текущую страницу
		ld a, (v_page)
		ld b, a

		; В начале файла должно быть D3,D3,D3
		ld hl, a_defHeader
		call	memcmp_hl_de_3	 ; b не изменяет

		; Если маркер найден, то переходим к fileLoadHeader2
		jp z, fileLoadHeader2

		; Не найден маркер начала файла!

		; Восстаналиваем заголовок в памяти
		ld hl, a_defHeader
		ld de, v_header
		call	memcpy_de_hl_15

		; Устаналиваем конец страницы
		ld hl, (f_nextFile)
		ld (v_pageFreePtr), hl
		ex de, hl

		; Получаем в HL размер страницы
		ld hl, (v_aramPageEnd)
		ld a, (a_panelOA)
		cp 'O'
		jp nz, fileLoadHeader_0
		 ld hl, ROM_PAGE_END
fileLoadHeader_0:	
                ; Рассчитываем свободное место на диске
		call	sub_hl_de
		ld (v_pageFreeTotal), hl

		; ?
		dec c
		ret
