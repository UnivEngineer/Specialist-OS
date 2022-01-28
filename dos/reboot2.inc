;+---------------------------------------------------------------------------
; MXOS
; Перезагрузка (продолжение)
;
; 2013-12-12 Дизассемблировано vinxru
;----------------------------------------------------------------------------

reboot2:
		; Очищаем экран
		call	clearScreen

		; Проверяем, нажата ли какая нибудь клавиша
		call	j_keyScan

		push	psw

		; Выводим версию ОС на экран
		lxi h, txtBiosVer
		call printString1

		; Определяем размер ДОЗУ и печатаем процесс на экран
		lxi	 h, ARAM_PAGE_END
		lda	 v_maxRamPage
		call checkARAM

		; Выводим объем памяти на экран
		lxi  h, txtRAM
		call printString1

		lda	 v_maxRamPage
        call printHexByte

		lxi  h, txtKB
		call printString1

        ;call getch

		pop	psw

		; Если не нажата ни одна клавиша, запускаем файл с RAM-диска
		inr	a
		jz	j_reboot3

		; Загружаем программу с ленты (без контроля КС) и запускаем
		call	tapeLoadInt
		lhld	v_tapeAddr
		pchl

; ---------------------------------------------------------------------------

txtBiosVer:
		.db 0Ch	; '\f'
		.text "MXOS BIOS 4.50"
		.db 0Ah	; '\n'
		.db 0

txtRAM:
		.db 0Ah	; '\n'
		.text "RAM "
		.db 0

txtKB:
		.text " * 64 KB"
		.db 0Ah	; '\n'
		.db 0
