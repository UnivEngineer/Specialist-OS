;+---------------------------------------------------------------------------
; RAMFOS
; Стандартная точка входа F851
; Удаление файла найденного программой F84B
;
; Файл должен быть обязательно найден функцией fileFind (F84B), иначе
; будет повреждена структура файлов.
;
; 2013-11-01 Дизассемблировано vinxru
;----------------------------------------------------------------------------

fileDelete:	 ; Расчет длины полезных данных после нашего файла, которые
		; надо сдвинуть.
		; de = v_pageFreePtr - f_nextFile - 1 
		ld hl, (f_nextFile)
		ex de, hl
		ld hl, (v_pageFreePtr)
		dec hl
		call	sub_hl_de
		ex de, hl

		; Помещаем в стек адрес следующего файла
		push hl

		; Помещаем в HL адрес нашего файла
		ld hl, (v_curFile)

		; Адрес следующего файла изменится после сдвига,
		; а он используется в функции compareFileName
		ld (f_nextFile), hl

		; Если наш файл был посленим, ничего не перемещаем
		jp c, fileDelete_1

fileDelete_0:	  ; Чтение с диска
		 ex (sp), hl
		 call	pageRead
		 inc hl
		 ex (sp), hl
		 ; Запись на диск
		 call	pageWrite
		 inc hl
		 ; if(de==0) break;
		 ld a, e
		 or d
		 ; de--
		 dec de
		jp nz, fileDelete_0

fileDelete_1:	 ; Обновляем счетчик свободного места
		ex (sp), hl
		pop hl
		jp fileChanged
