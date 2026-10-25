;+---------------------------------------------------------------------------
; RAMFOS
; Запуск RKS файла
;
; 2013-11-01 Разработано vinxru
;----------------------------------------------------------------------------

    IF RKS_LOADER
		; Это RKS файл
		ld hl, a_rks
		call	memcmp_ext
		jp nz, defExec2
		
		; Загрузить его в память
		call	fileLoad2
		jp nz, dlg_error

		; Адрес запуска
		push hl
    IF RKS_SHIFT_LOADER
		; Если нажат HP
		ld a, (IO_KEYB_B)
		and 2
		ret z
    ENDIF
		; Найти файл Monitor2.sys
		ld hl, a_monitor2_sys
		call	exec1

		; Адрес запуска
		pop hl

		jp nz, dlg_error
		
		; При загрузке монитора стандартный стек будет уничтожен
		ld sp, 8FFFh

		; Адрес программы
		push hl
		jp exec2

a_rks:	DB "RKS"
a_monitor2_sys:	DB "Monitor2 SYS"

defExec2:
    ENDIF
