;+---------------------------------------------------------------------------
; RAMFOS
; Установить активную страницу
;
; На входе
;  b - диск
;
; На выходе
;  v_pageStart = v_curFile = f_nextFile = начало страницы, заголовок первого файла
;  hl - начало страницы
;
; 2013-11-01 Дизассемблировано vinxru
;----------------------------------------------------------------------------

fileSetPage:	 ; Начало RAM-диска
		ld hl, (v_aramPageStart)

		; Диск
		ld a, b
		ld (v_page), a

		; У ROM-диска другое начало
		; Обновляем надпись RAM/ROM в заголовке файловой панели
		or a
		ld a, 'A'
		jp p, fileSetPage_1
		 ld a, 'O'
		 ; Начало ROM-диска
		 ld hl, ROM_PAGE_START
fileSetPage_1:	ld (a_panelOA), a

                ; Параметры диска
		ld (v_pageStart), hl
		ld (v_curFile), hl
		ld (f_nextFile), hl
		ret		
