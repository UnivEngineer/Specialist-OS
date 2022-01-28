;+---------------------------------------------------------------------------
; MXOS
; Запустить BAT файл. Вызывается функцией fileExec
;
; 2013-12-12 Дизассемблировано vinxru
;----------------------------------------------------------------------------

execBat:
		pop	d

		; Устанавливаем адрес загрузки BAT-файла на diskDirectory
		lxi  h, diskDirectory
		shld v_batPtr
		xchg

		; Изменяем адрес загрузки BAT-файла
        ; TODO: это не работает с ром-диском (A: без поддержки записи). Поэтому
        ; нельзя запустить BAT-файл с диска A:, если его адрес загрузки в каталоге
        ; уже не прошит как FC00h (= &diskDirectory)
		lxi  h, v_fileName
		mvi  c, 1
		call fileGetSetAddr
		jc   badCommand

		; Сохраняем имя BAT-файла
		lxi  h, v_fileName	
		lxi  d, v_fileName_end
		lxi  b, v_batFileName
		call memmove_bc_hl

		; Сохраняем диск содержащий BAT-файл
		lda	v_drive
		sta	v_batDrive+1 ; самомодификация кода

        ; Этот цикл выполняется для каждой строки BAT-файла
execBat_loop:

		; Сохраняем текущий диск в регистре B
		lda  v_drive
		mov  b, a

v_batDrive:
        ; Выбрать диск содержащий BAT файл
		mvi  a, 1 ; << сюда вместо 1 пишется диск, содержащий BAT файл
		call fileSelectDrive

		; Загрузить BAT-файл в память
		lxi  h, v_batFileName 
		call fileLoad
		lhld v_batPtr

		; Сохраняем начало строки
		mov	d, h
		mov	e, l

        ; Ищем конец строки
execBat_0:
		mov	a, m		
		inx	h

		; Найден конец строки, запускаем файл
		cpi	0Dh		
		jz	execBat_1

		; Конец	файла
		cpi	0FFh		
		jnz	execBat_0
		ret

; ---------------------------------------------------------------------------

execBat_1:
        ; Сохраняем указатель
		shld v_batPtr	

		; Восстановление активного диска		
		mov  a, b		
		call fileSelectDrive

		; Запустить файл
		xchg			
		call fileExec

		jmp execBat_loop

