;+---------------------------------------------------------------------------
; RAMFOS
; Стандартная точка входа F800
; Инициализация
;
; 2013-11-01 vinxru
;----------------------------------------------------------------------------

    IF MXOS_KERNEL
init:   jp compatReturn
    ELSE
init:		 ; Включение ОЗУ
		ld (IO_PAGE_RAM), a
	
		; Инициализация	стека
		ld sp, STACK_TOP

		; Инициализация BIOS
		call	j_biosInit

		; Верхушка памяти
		ld hl, RAM_TOP
		call	j_setMemTop

		; Фильтр
		ld hl, a_12spaces + 9
		call	fileSetType

		; Диск
		ld b, DLG_START_PAGE

		; Продолжается в dlg_main
    ENDIF
