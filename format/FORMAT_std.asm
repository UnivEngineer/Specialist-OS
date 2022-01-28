;+---------------------------------------------------------------------------
; MXOS
; FORMAT.COM
;
; 2022-01-14 Disassembled by SpaceEngineer
;----------------------------------------------------------------------------

.org 0D000h

; Функции DOS
getch			= 0C803h	; Ожидание ввода с клавиатуры
printChar		= 0C809h	; Вывод символа на экран
printString		= 0C818h	; Вывод строки на экран
fileGetSetDrive	= 0C842h	; Получить/установить активное устройство
diskDriver		= 0C863h	; Драйвер выбранного диска

Buffer          = 0D100h	; Буфер

; Код

		ldax    d	; В DE передаётся адрес строки аргументов
		cpi     20h
		jnc		LetterEntered

		; Запрос буквы диска для форматирования
ChooseDrive:
		lxi     h, str_ChoseDrive
		call    printString		; Вывод сообщения 'CHOOSE DRIVE ? '
		call    getch			; Ожидание нажатия клавиши
		cpi     21h				; Сравнение с пробелом
		rc
		mov     c,a
		call    printChar

LetterEntered:
		sta     str_A_Y_N		; Заменим 'A' в строке на введённую букву
		sui     41h				; Номер диска
		cpi     08h				; Максимальный номер диска = 7
		jnc     ChooseDrive		; Повтор запроса буквы, если неверная
		mov     b, a			; Запомним номер диска в B

		lxi     h, str_Format
		call    printString		; Вывод сообщения 'FORMAT B: Y/N'
		call    getch			; Ожидание нажатия клавиши
		cpi     59h				; Сравнение с 'Y'
		rnz						; Выход, если не 'Y'

		; Установим выбранный диск текущим
		mov     a, b			; Номер диска в A
		mvi     e, 01h
		call    fileGetSetDrive

		; Выдать размер диска в A
		mvi     e, 03h
		call    diskDriver
		mov     e, a	; Помещаем размер диска в E
		dcr     a

		; Очистка буфера (E байт)
		lxi     h, Buffer
ClearBufLoop:
		mvi     m, 0
		inr     l
		dcr     e
		jnz     ClearBufLoop

		; Создание пустой структуры FAT (256 байт)
CreateFATLoop:
		inr     a
		jz      WriteToDisk
		mvi     m, 01h
		inr     l
		jmp     CreateFATLoop

		; Запись FAT на диск
		; D - номер сектора
		; E - код операции
WriteToDisk:
		lxi     d, 0001h  ; Запись сектора номер 0
		call    diskDriver

		; Создание пустой структуры каталога (256 байт)
CreateCatLoop:
		mvi     m, 0FFh
		inr     l
		jnz     CreateCatLoop

		; Запись секторов с номера 3 по 1
		mvi     d, 03h
WriteLoop:
		call    diskDriver
		dcr     d
		jnz     WriteLoop

		; Выход в ОС
		ret

; Данные

str_Format:
		.db 0Ah
		.text "FORMAT "

str_A_Y_N:
		.text "A: Y/N "
		.db 0

str_ChoseDrive:
		.db 0Ah
		.text "CHOOSE DRIVE ? "
		.db 0

.end
