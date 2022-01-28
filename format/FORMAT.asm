;+---------------------------------------------------------------------------
; MXOS
; FORMAT.COM
;
; 2022-01-14 Дизассемблировано и доработано SpaceEngineer
;
; Доработки:
; - "тихий режим" по параметру Y (форматировать без спроса)
;    например FORMAT.COM B: Y
;
;----------------------------------------------------------------------------

; Функции DOS
getch			= 0C803h	; Ожидание ввода с клавиатуры
printChar		= 0C809h	; Вывод символа на экран
printString		= 0C818h	; Вывод строки на экран
fileGetSetDrive	= 0C842h	; Получить/установить активное устройство
diskDriver		= 0C863h	; Драйвер выбранного устройства

Buffer          = 0D100h	; Буфер

;----------------------------------------------------------------------------

.org 0D000h

		ldax    d		; В DE передаётся адрес строки аргументов
		cpi     20h
		jnc		ReadParams	; Прыжок, если есть параметр

		; Запрос буквы диска для форматирования
ChooseDrive:
		lxi     h, str_ChoseDrive
		call    printString		; Вывод сообщения 'CHOOSE DRIVE: '
		call    getch			; Ожидание нажатия клавиши
		mov     c, a
		call    printChar
		cpi     21h				; Сравнение с пробелом
		jc		Abort			; Выход в ОС, если меньше или равно
		mov     b, a			; Запомнить букву диска в B
		jmp     ConfirmRequest

ReadParams:
		mov     b, a			; Запомнить букву диска в B

SearchLoop1:					; Поиск первого пробела в строке параметров
		ldax    d
		cpi     21h
		jc		SearchLoop2
		inx     d
		jmp     SearchLoop1

SearchLoop2:					; Пропуск последующих пробелов
		ldax    d
		cpi     20h
		jnz		SearchLoopExit
		inx     d
		jmp		SearchLoop2

SearchLoopExit:		
		cpi     'Y'				; Если найден параметр 'Y', переход к форматированию
		jz		Confirmed

		; Подтверждение форматирования
ConfirmRequest:
		mov     a, b
		sta     str_A_Y_N		; Заменить 'A' в строке сообщения на введённую букву
		lxi     h, str_Format
		call    printString     ; вывод сообщения 'FORMAT <буква>: [Y/N]?'
		call    getch			; Ожидание нажатия клавиши
		mov     c, a
		call    printChar
		cpi     'Y'				; Сравнение с 'Y'
		jnz		Abort			; Выход в ОС, если не 'Y'
		
Confirmed:
		mov     a, b			; Восстановить букву диска в A

		; Буква диска в регистре A
Format:
		sui     41h				; Номер диска
		cpi     08h				; Максимальный номер диска = 7
		jnc     InvalidDrive	; Выход, если неверный номер диска
		mov     b, a			; Запомнить номер диска в B

		; Установить выбранный диск текущим
		mov     a, b	; Номер диска в A
		mvi     e, 01h
		call    fileGetSetDrive

		; Выдать размер диска в A
		mvi     e, 03h
		call    diskDriver
		mov     e, a	; Поместить размер диска в E
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

Abort:
		lxi     h, str_Aborting
		call    printString		; Вывод сообщения 'ABORTING'
        ret

InvalidDrive:
		lxi     h, str_InvalidDrive
		call    printString		; Вывод сообщения 'INVALID DRIVE LETTER'
        ret

;----------------------------------------------------------------------------
; Данные

str_Format:
		.db 0Ah
		.text "FORMAT "

str_A_Y_N:
		.text "A: [Y/N]? "
		.db 0

str_ChoseDrive:
		.db 0Ah
		.text "CHOOSE DRIVE: "
		.db 0

str_InvalidDrive:
		.db 0Ah
		.text "INVALID DRIVE LETTER"
		.db 0

str_Aborting:
		.db 0Ah
		.text "ABORTING"
		.db 0

;----------------------------------------------------------------------------

.end
