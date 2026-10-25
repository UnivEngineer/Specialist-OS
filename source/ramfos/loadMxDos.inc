;+---------------------------------------------------------------------------
; RAMFOS
; Загрузка и запуск MXDOS с дискеты
; Вызывается при нажатии на F6 из файловой панели
;
; 2013-11-01 Дизассемблировано vinxru
;----------------------------------------------------------------------------

    IF NO_MXDOS == 0

MXDOS_ADDR = 0E400h

loadMxDos:	ld b, a

		; Установка головки и сектора
		ld a, 1
		ld (IO_FLOPPY_HEAD), a
		ld (IO_FLOPPY_SECTOR), a

		; Пауза
		ld h, 13h
loadMxDos_0:	dec hl
		 ld a, l
		 or h
		jp nz, loadMxDos_0

		; Устанавливаем головку
		ld (IO_FLOPPY_HEAD), a

		; Адрес загрузки
		ld hl, MXDOS_ADDR

		; Чтения сектора
		ld a, 90h
		ld (IO_FLOPPY_CMD), a

		; Ожидание готовности
loadMxDos_1:	ld a, (IO_FLOPPY_CMD)
		 rla
		 ret c
		 rra
		 rra
		jp nc, loadMxDos_1

		; Получение данных
		ld c, 13h
loadMxDos_2:	ld (IO_FLOPPY_TRIG), a
		  ld a, (IO_FLOPPY_DATA)
		  ld (hl), a
		  inc l
		 jp nz, loadMxDos_2
		 inc h
		 dec c
		jp p, loadMxDos_2

		; Проверка
		ld a, (IO_FLOPPY_CMD)
		xor 1
		or a
		ld a, 0D0h
		ld (IO_FLOPPY_CMD), a
		jp nz, dlg_error

		; Проверка
		dec hl
		ld a, (hl)
		dec hl
		inc (hl)
		cp (hl)
		jp nz, dlg_error

		; Запуск MXDOS
		ld a, b
		call	MXDOS_ADDR

		; При выходе из MXDOS выбираем ROM-диск
		ld b, 80h
		ret
    ENDIF
