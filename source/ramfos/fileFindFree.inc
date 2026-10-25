;+---------------------------------------------------------------------------
; RAMFOS
; Расчет контрольной суммы сохраняемого файла, поиск страницы
; с подходящим cвободным объемом, удаление файла с именем v_header2_name
;
; На входе
;  v_header2_name  - Имя файла для удаления
;  f_header2_start - Адрес начала файла
;  f_header2_stop  - Адрес конца файла
;
; На выходе
;  jnc             - Страница найдена
;  f_header2_crc   - Контрольная сумма
;  v_page, b       - Страница
;
; 2013-11-01 Дизассемблировано vinxru
;----------------------------------------------------------------------------

fileFindFree:	 ; Расчитываем контрольную сумму в v_header2_crc
		ld (v_header2_start), hl
		ex de, hl
		ld (v_header2_end), hl
		ex de, hl
		call	j_calcCrc
		ld h, b
		ld l, c
		ld (v_header2_crc), hl

		; Поиск по всем страницам и удаление файла
		ld b, 20h
		call	fileFindEx

		; Поиск файла начиная с нулевой страницы
		ld b, 20h
fileFindFree_0:	  ; Определяем свободное место на странице
		 call	fileFind1
		 
		 ; de = расчитываем требуемый размер файла, почему то в цикле.
		 call	fileGetStartAndSize2
		 ld de, FILE_HEADER_SIZE + 3
		 add hl, de
		 ex de, hl

		 ; Выходим с флагом NC, если места достаточно
		 ld hl, (v_pageFreeTotal)
		 call nc, j_cmp_hl_de
		 ret nc

		 ; Следующая страница
		 call	pageNext
		jp nc, fileFindFree_0
		ret
