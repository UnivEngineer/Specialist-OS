;+---------------------------------------------------------------------------
; RAMFOS
; Инициализация BIOS
; Вызывается только функцией init
;
; 2013-11-01 Дизассемблировано и доработано vinxru
;----------------------------------------------------------------------------

biosInit:	 ; Очистка переменных начиная с v_dblClear
		ld hl, v_dblClear
		ld b, 47
		xor a
		ld (v_inverse), a
loc_C8EA:	ld (hl), a
		 inc hl
		 dec b
		jp nz, loc_C8EA

		; Инициализация	контроллера цвета черным цветом для очистки экрана
    IF INIT_SCREEN_CUR_COL == 1
		ld a, (v_selColor)
    ELSE
		ld a, INIT_SCREEN_COLOR
    ENDIF
		call	setColor

		; Инициализация	клавиатуры
		call	keyMode82	

		; Скрываем нижнюю строку
		call	bottomLineHide

		; Очищаем экран
		call	clearScreen
		
		; Настройка таймера
		ld hl, IO_TIMER+3

;----------------------------------------------------------------------------

		ld (hl), 36h
		ld (hl), 76h
		ld (hl), 90h
		dec hl
		ld (hl), 10h
		dec hl
		ld (hl), 20h
		ld (hl), 4Eh
		dec hl
		ld (hl), 10h
		ld (hl), 15h
		
    IF MXOS_KERNEL == 0
		; *** Определение размера ДОЗУ ***

    IF WORK_WITHOUT_ARAM
		; Надо вернуть доступ к ДОЗУ, иначе проверка не работает
		ld a, 0FDh ; Порт выбора ДОЗУ		
		ld (disableARAM1+1), a
		ld (disableARAM2+1), a
    ENDIF

    IF TRUE_CHECK_ARAM
		ld hl, ARAM_PAGE_END
    ELSE
		ld hl, (v_aramPageEnd) ; Это не совсем правильно, так как при программном сбросе проверка будет работать не так, как при аппаратном
    ENDIF
		ld a, (v_aramMaxPageI)
		call	checkARAM
		
    IF WORK_WITHOUT_ARAM
		ld de, 0 ; Начало ДОЗУ
    ENDIF
		; Если ДОЗУ найдено
    IF TRUE_CHECK_ARAM
		jp nz, aramFounded ; Так короче код испрваленной функции
    ELSE
		jp z, aramFounded
    ENDIF

		; ДОЗУ не найдено
    IF WORK_WITHOUT_ARAM
		 ; Настраиваем небольшой диск в основном ОЗУ
		 ld de, miniPage
		 ld hl, miniPageEnd
		 ld a, 0FCh ; Порт выбора ОЗУ
		 ld (disableARAM1+1), a
		 ld (disableARAM2+1), a
    ELSE
		 ; Пытаемся работать с 48 Кб ДОЗУ
		 ld hl, 0BFFBh
    ENDIF
aramFounded:	
		ld (v_aramPageEnd), hl
		ld (v_aramPageEnd2), hl
    IF WORK_WITHOUT_ARAM
		ex de, hl
		ld (v_aramPageStart), hl
    ENDIF
    ENDIF
		; *** Инициализация экрана ***
		ld hl, a_initScreen
		; Далее printString


		; Этот текст будет встроен в конец кода
    IF NICE
    MACRO A_INITSCREEN
a_initScreen: DB C_INVERSE, C_TOPLINE, ' ', 0F3h, 0D0h, 0C5h, 0C3h, 0C9h, 0C1h, 0CCh, 0C9h, 0D3h, 0D4h, ' ','M','X','2',' ','R','A','M','F','O','S',C_NORMAL,0
    ENDM
    ELSE                                                                                                                                                                  
    MACRO A_INITSCREEN
a_initScreen: DB C_INVERSE, C_TOPLINE, ' ', 0F3h, 0D0h, 0C5h, 0C3h, 0C9h, 0C1h, 0CCh, 0C9h, 0D3h, 0D4h, '_','M','X',92,'R','A','M','F','O','S',92,C_NORMAL,0
    ENDM
    ENDIF
