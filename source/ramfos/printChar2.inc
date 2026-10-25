;+---------------------------------------------------------------------------
; RAMFOS
; Обработка специальных кодов символов
;
; 2013-11-01 Дизассемблировано vinxru
;----------------------------------------------------------------------------

printChar2:	sub 21h
		jp nc, setColorChar
		add a, 19h
		jp z, popColor ; 87h Восстановние кода цвета из стека
		dec a
		jp z, pushColor ; 88h Запоминание кода цвета по принципу стека (глубина 2 цвета)
		dec a
		jp z, inverseColor ; 89h Инверсия кода цвета
		dec a
		jp z, changeInverse ; 8Ah Инверсия фона чернобелого экрана (инверсия инверсии)
		dec a
		jp z, setInverseNeg ; 8Bh Установка светлого фона черно-белого экрана (потому что A=0 будет инвертировано)
		dec a
		jp z, setInverse ; 8Сh Установка темного фона черно-белого экрана (потому что A=0)
		dec a
		jp z, pushCursorLine ; 8Dh Запоминание курсора строки
		dec a
		jp z, popCursorLine ; 8Eh Восстановление курсора строки
		dec a
		jp z, clearTopLine ; 8Fh Очистка верхней служебной строки
		dec a
		jp z, clearBottomLine ; 90h Очистка нижней служебной строки
		dec a
		jp z, scrollDown ; 91h Сдвииг экрана вних от курсора
		dec a
		jp z, scrollUp ; 92h Сдвин экрана вверх от курсора
		dec a
		jp z, pushCursor ; 93h Запоминание курсора экрана
		dec a
		jp z, popCursor ; 94h Восстановление курсора экрана
		dec a
		jp z, printFileName ; 95h Вывод имени файла загруженного с диска
		dec a
		ex de, hl
		ld hl, (v_koi8)
		dec a
		jp z, c_koi7 ; 97h Включение режиме KOI-7
		dec a
		jp z, c_koi8 ; 98h Включение режиме KOI-8
		dec a
		dec a
		ret nz ; Выход, а дальше 9Ah Сдвиг экрана вверх от нижней служеюной строки до текущей строки курсора
		ex de, hl
		push hl
		inc l
		inc l
		ld a, l
		add a, 10
		ld e, a
		ld a, (v_screenHeight)
		jp c_scrolUpEx
