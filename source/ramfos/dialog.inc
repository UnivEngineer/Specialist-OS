;----------------------------------------------------------------------------
; RAMFOS
; Файловая панель
;
; 2013-11-01 Дизассемблировано vinxru
;----------------------------------------------------------------------------

    IF ALL_CHARS
    DEFINE BORDER_BL  27,12
    DEFINE BORDER_BL1 12
    DEFINE BORDER_B   27,13
    DEFINE BORDER_B1  13
    DEFINE BORDER_BR  27,24
    DEFINE BORDER_BR1 24
    DEFINE BORDER_L   27, 8
    DEFINE BORDER_L1  8
    DEFINE BORDER_R   27, 10
    DEFINE BORDER_R1  10
    DEFINE BORDER_H   27, 25
    DEFINE BORDER_H1  25
    ELSE
BORDER_BL  = 15h
BORDER_BL1 = 15h
BORDER_B   = 14h
BORDER_B1  = 14h
BORDER_BR  = 16h
BORDER_BR1 = 16h
BORDER_L   = 11h
BORDER_L1  = 11h
BORDER_R   = 6
BORDER_R1  = 6
BORDER_H   = 6
BORDER_H1  = 6
    ENDIF

dlg_loadFile:	 ; Активная страница
		ld a, (v_page)
		and 0Fh
		; Режим выбора файла (6 ой бит)
		or 40h
		ld b, a
		jp dlg_main

; ---------------------------------------------------------------------------

    INCLUDE "fileSetAddr.inc"
    INCLUDE "fileSave.inc" ; Продолжение
    INCLUDE "fileChanged.inc"
    INCLUDE "fileFindFree.inc"

; ---------------------------------------------------------------------------
; УДАЛЕНИЕ ДАТЫ

dlg_f2_alt:	 ; Если режим выбора файла, то выходим
		ld a, b
		rla
		rla
		ret c

		; Вывод сообщения "дату удалить"
		ld de, a_deleteDate
		call	dlg_msgYesNo

    INCLUDE "deleteDate.inc"

		; Далее init

; ---------------------------------------------------------------------------

    INCLUDE "init.inc" ; Продолжение в dlg_main

; ---------------------------------------------------------------------------

dlg_main:	
		; Очищаем экран, перерисовываем заголовок и нижнюю строку
		call	dlg_pageChanged
	
dlg_main_0:
		; Сохраняем текущий файл и положение курсора
    IF DLG_CURSOR_AT_TOP
		ex de, hl
		ld hl, (v_curFile)
		push hl
		ld hl, (v_cursorPos)
		push hl
		ex de, hl
    ENDIF	
		; Выводим все файлы на экран
dlg_main_1:	 call	fileLoadHeader
		 push af
		  call	dlg_printFile
		 pop af
		 ld hl, (f_nextFile)
		jp z, dlg_main_1

    IF NICE
dlg_z1:
		call	getCursorPos
		ld a, h
		sub 16h
		jp nc, dlg_z0
		; 46 пробелов
		ld bc, (46 << 8) + ' '
		call	printChars	
		; Правая граница
		call	dlg_rightEdge
		jp dlg_z1
dlg_z0:		
    ENDIF		

		; Рисуем нижнюю границу
		call	dlg_bottomEdge
		; Восстанавливаем положение курсора
    IF DLG_CURSOR_AT_TOP
		pop hl
		ld (v_cursorPos), hl
		pop hl
		ld (v_curFile), hl
    ELSE
		ld hl, (v_printedFile)
    ENDIF	
		call    dlg_reloadItem

		call    printStringEx
    IF NICE == 0
		DB C_BEEP
    ENDIF
		DB C_POPCURSORL, C_PUSHCURSORS, C_NORMAL, 0

		ld hl, (v_curFile)
		jp dlg_main_0

; ---------------------------------------------------------------------------

dlg_msgYesNo:	 ; Вывод сообщения
		call	dlg_msg

dlg_yesNo:	 ; Ввод строки из 4-х символов
		ld hl, buffer
		ld de, 4FFh
		call	j_input
		ld (hl), a

		; Если нажат ENTER, печатаем символ 0E и выходим
		ld a, 0Eh
		jp c, j_printCharA ; Выход

		; Если нажат не ESC, повторяем
		jp nz, dlg_yesNo

		; Выкидываем адрес возрата
		pop hl
		
loc_F9D0:	 ; Стираем то, что нарисовали
		call	dlg_bottomEdge

		; Перезагрузить и перерисовать выбранный файл
		ld hl, (v_curFile)
		jp dlg_reloadItem

; ---------------------------------------------------------------------------
; Рисование нижней границы

dlg_bottomEdge:
		; Темный фон, восстановление курсора экрана, удаление, какой то символ
		call	printStringEx
		DB 8Ch, 94h, 8, BORDER_BL, 0
	
		; 46 символов 0x14
		ld bc, 2E00h | BORDER_B1

		call	printChars

		; Символ 16, восстановление курсора строки
		call	printStringEx
		DB BORDER_BR, 8Eh, 0

		ret

; ---------------------------------------------------------------------------
; Клавиша вниз.

dlg_down:	 ; Если это последний файл, переходим на следующую страницу
		call	dlg_isLastLine
		jp nc, dlg_nextPage

		; Если это последняя строка на экране, перерисовываем экран
		; так что наша строка оказывается сверху.
		ld a, (v_dlgOffset+1)
		dec a
		cp c
		jp z, dlg_drawItems

		; Помещаем в стек адрес следующего файла
		ld hl, (f_nextFile)
		push hl

		; Стираем курсор
		call	printFileHeaderDark
		
		jp dlg_upDownInt
		
; ---------------------------------------------------------------------------

dlg_nextPage:	 ; Следующая страница (в регистре b)
		call	pageNext

		; Это была последняя страница
		ret c

		; Страница была изменена
		jp dlg_pageChanged

; ---------------------------------------------------------------------------

dlg_prevPage:	 ; Если это нулевая страница, выходим
		ld a, b
    IF ARAM_MAX_PAGE <= 7
		and 7
    ELSE
		and 0Fh ; Старшие биты выполняют спец функции
    ENDIF
		jp z, dlg_drawItem

		; Уменьшаем страницу
		dec b
		jp dlg_pageChanged

; ---------------------------------------------------------------------------

dlg_up:		 ; Если это первая строка, то перейти на предыдущую страницу
		ld hl, (v_curFile)
		call	j_cmp_hl_de
		jp z, dlg_prevPage

		; Непонятно что в с
		ld a, c
		cp 3
		jp z, dlg_pageChanged

    INCLUDE "filePrev.inc"

		; Сохраняем адрес файла
		push bc

dlg_upDownInt:	 ; Стираем курсор
		call	printFileHeaderDark

dlg_upDownInt2:
		; Перемещаем курсор вверх или вниз
		ld a, (v_lastKey)
		call	j_printCharA

		; Восстанавливаем HL
		pop hl

dlg_reloadItem:	 ; Загрузить заголовок файла
		call	fileLoadHeader

dlg_drawItem:	 ; Вывести название файла
		call	printFileHeaderLight
		ld c, d

		; Ждем символ с кодом <0x20
dlg_0:		call	keyWaitNoCursor
		cp 20h
		jp nc, dlg_0

		ld (v_lastKey), a

		; de - начало диска
		ld hl, (v_pageStart)
		ex de, hl

		cp 19h
		jp z, dlg_up
		cp 1Ah
		jp z, dlg_down
		cp 0Dh
		jp z, dlg_enter
		cp 1Bh
		jp z, dlg_esc ; ESC
		or a
		jp z, dlg_f1 ; F1
		dec a
		jp z, dlg_f2 ; F2
		dec a
		jp z, dlg_f3 ; F3
		dec a
		jp z, dlg_f4 ; F4
		dec a
		jp z, dlg_f5 ; F5
		dec a
		jp z, dlg_f6 ; F6
		dec a
		jp nz, dlg_drawItem
		; Продолжение в startDebuger

; ---------------------------------------------------------------------------

    INCLUDE "startEditorDebuger.inc"
    INCLUDE "keyWaitNoCursor.inc"
    INCLUDE "memcmp_hl_de_c.inc"

; ---------------------------------------------------------------------------

dlg_enter:	call	dlg_isLastLine
		jp z, dlg_drawItem

    IF OPEN_ANY_FILE == 0
		; Сравнить расширение выбранного файла с установленным расширением
		ld hl, v_header2_ext
		call	memcmp_ext
    ENDIF
		; Если это режим выбора файла
		ld a, b
		rla
		rla
    IF OPEN_ANY_FILE
		jp c, dlg_enter_1
    ELSE
		jp c, dlg_enter_0
    ENDIF	

    INCLUDE "rks.inc"

		; Допускаются только расширения EXE и COM
		ld hl, a_exe ; "EXE"
		call	memcmp_ext
		ld hl, a_com ; "COM"
		call nz, memcmp_ext

		; Недопустимое расширение, переходим в основной цикл
dlg_enter_0:	jp nz, dlg_drawItem

dlg_enter_1:	ld hl, (v_header_start)
		ex de, hl

		; Режим выбора
		jp c, dlg_select

		call	fileLoadComEx
		jp nz, dlg_error

		;
		ex (sp), hl
		jp loc_FB05

; ---------------------------------------------------------------------------

dlg_f1:		 ; Выводим сообщение
		ld de, a_copy
		call	dlg_checkMsg
		ret z

		; Ввод числа в v_header_start
		call	dlg_yesNo
		ld hl, (v_header_start) ; Зачем?
		call m, j_strToHex ; Преобразовать строку DE в число HL
		jp z, dlg_error

fileLoadSetAddr: ; Установить новый адрес загрузки
		; de = v_header_start = hl; 
		; hl = v_header_stop  = f_fileBodySize + hl;
		call	fileChangeAddr

		; Если режим выбора файла, то сохраняем заголовок файла.
		; В заголовке мы только что изменили адрес загрузки
		ld a, b
		rla
		rla
		jp nc, fileSaveHeader
		
dlg_select:	 ; Загрузить файл в память
		call	fileLoadComEx
		jp nz, dlg_error
		pop hl

loc_FB05:	push bc
		 ; Вывести имя файла
		 call	printFileHeaderName
		 ; Сохранить информацию о загруженном файле
		 call	memcpy_fd_fd2
		pop bc

loadDStartDStop: ; de=v_header_stop, hl=v_header_start
		ld hl, (v_header_stop)
		ex de, hl
		ld hl, (v_header_start)
		ret

; ---------------------------------------------------------------------------

; de = v_header_start = hl; v_header_stop = f_fileBodySize+hl;

fileChangeAddr:	ld (v_header_start), hl
		ex de, hl
		ld hl, (f_fileBodySize)
		add hl, de
		ld (v_header_stop), hl
		ret

; ---------------------------------------------------------------------------

    INCLUDE "fileLoad.inc"

; ---------------------------------------------------------------------------

dlg_esc:	 ; Если это не режим выбора файла, ничего не делаем
		ld a, b
		rla
		rla
		ret nc

		; Возвращаемся в программу		
		pop hl
		ccf
		ret

; ---------------------------------------------------------------------------

    INCLUDE "fileLoadHeader.inc"

; ---------------------------------------------------------------------------
; Скопировать имя выбранного файла с v_header2_name

memcpy_fd_fd2:	ld hl, v_header
		ld de, v_header2
memcpy_de_hl_15:	ld c, 14

    INCLUDE "memcpy_de_hl_c.inc"
    INCLUDE "fileLoadHeader2.inc"

; ---------------------------------------------------------------------------

printFileHeaderLight:
		; Если сделать курсор цветным, то цвет будет задевать рамку. 

		; Инверсия
		ld a, C_INVERSE
		call	j_printCharA

printFileHeaderDark:
		; Функция сохранит положение курсора строки, но не восстановит его
		; Выведет на экран имя файла
		; После вывода имени функция установит темный фон
		call	printFileHeaderName2

popCursorLine2:	 ; Восстановить курсор строки
		ld a, C_POPCURSORL
		jp j_printCharA

; ---------------------------------------------------------------------------
; Функция сохранит положение курсора строки, но не восстановит его
; Выведет на экран имя файла
; После вывода имени функция установит темный фон

printFileHeaderName2:
		call	pushCursorLine2
		call	j_getCursorPos
		ex de, hl

printFileHeaderName:
		ld c, 12
		ld hl, v_header_name
printFileHeaderName_0:
		 ld a, (hl)
		 inc hl
		 call	j_printCharA
		 dec c
		jp nz, printFileHeaderName_0

		; Установка темного фона
		ld a, C_NORMAL
		jp j_printCharA

; ---------------------------------------------------------------------------

dlg_printFile:	push af
		 call	j_getCursorPos
		 ld a, h
v_dlgOffset:	sub 16h
		pop hl

		; if(a >= 0x16)	return;
		ret nc
		
		; D272 = v_curFile
		push hl
    IF DLG_CURSOR_AT_TOP == 0
		 ld hl, (v_curFile)
		 ld (v_printedFile), hl
    ENDIF		
		pop af

		; Если на входе в функцию был NZ
		jp nz, loc_FC7A

		; Вывести имя файла
		call	printFileHeaderName2

		; Разделитель
		call	dlg_printSep

		; Если файл изменен
		ld a, (hl)
		call	j_printCharA
		inc hl

		; Дата
		ld c, 3
loc_FC1E:	ld a, (hl)
		 inc hl
		 call	j_printHex
		 dec c
    IF NICE
		 jp z, loc_FC1E_
		 ld a, '-'
		 call	j_printCharA
		jp loc_FC1E
loc_FC1E_:
    ELSE
		jp nz, loc_FC1E
    ENDIF
		
		; Разделитель
		call	dlg_printSep

		; Информация о файле
		call	dlg_printFileInfo

		; Выводим длину
		ld hl, (f_fileBodySize)
		inc hl

printNumber:			
		; Разделитель
		call	dlg_printSep

		; Вывод	HL на экран в 10чном виде
		ld de, 10000
		call	divAndPrint
		ld de, 1000
		call	divAndPrint
		ld de, 100
		call	divAndPrint
		ld de, 10
		call	divAndPrint
		ld a, l
		add a, '0'
		call	j_printCharA
		
		; Правая граница
dlg_rightEdge:
    IF ALL_CHARS
		ld c, BORDER_R1
		call	printCharAll
    ELSE
		ld a, BORDER_R1
		call	j_printCharA
    ENDIF	
		; Правый край
		ld hl, a_rightEdge
		jp j_printString

; ---------------------------------------------------------------------------

dlg_printFileInfo:		
		call	loadDStartDStop	 ; de=v_header_stop, hl=v_header_start
    IF NICE
		ld c, '-'
    ELSE
		ld c, ','
    ENDIF
		call	printHexWord	 ; printHexWord(v_header_start)
		ex de, hl
    IF NICE
		ld c, ' '
    ELSE
		inc c ; c = '-'
    ENDIF
		call	printHexWord	 ; printHexWord(v_header_stop)
    IF NICE
		ld c, BORDER_H1
		call	printCharAll
    ENDIF
		ld hl, (v_header_crc) ; c = ?
		ld c, C_NORMAL
		; Продолжение на printHexWord

; ---------------------------------------------------------------------------

    INCLUDE "printHexWord.inc"

; ---------------------------------------------------------------------------

loc_FC7A:	 ; Функция сохранит положение курсора строки, но не восстановит его
		; Выведет на экран имя файла
		; После вывода имени функция установит темный фон
		call	printFileHeaderName2

    IF NICE
		; 27 пробелов
		ld bc, 1B20h ; b-колво, с-символ
    ELSE
		; 26 пробелов
		ld bc, 1A20h ; b-колво, с-символ
    ENDIF
		call	printChars	
		
		; Свободное место
		ld hl, (v_pageFreeTotal)
		jp printNumber

; ---------------------------------------------------------------------------

    INCLUDE "printChars.inc"

; ---------------------------------------------------------------------------

divAndPrint:	ld c, '0'-1
loc_FC93:	inc c
		 call	sub_hl_de
		jp nc, loc_FC93
		add hl, de
		jp j_printChar
		
; ---------------------------------------------------------------------------

    INCLUDE "filesetpage.inc"

; ---------------------------------------------------------------------------

dlg_f6:		
    IF NO_MXDOS == 0
		; Если в номере диска 7, 6 биты не установлены, запускаем MXDOS
		ld a, b
		and 0C0h
		call z, loadMxDos
    ENDIF
		; Запустить не удалось, меняем на противоположный 7 бит
		ld a, b
    IF NO_ROM_SWITCH_BUG
		and 0F0h
    ENDIF
		xor 80h
		ld b, a

dlg_pageChanged: ; Устаналиваем активную страницу
		call	fileSetPage

dlg_drawItems:				 ; CODE XREF: dlg_down+Bj
		push hl
		 ; Копируем в заголовок установленный фильтр
		 ld hl, v_header2_ext
		 ld de, a_panelFilter
		 ld c, 2
		 call	memcpy_de_hl_c	 ; memcpy(de, hl, c+1)

		 ; Копируем туда же активную страницу
		 ld a, b
    IF ARAM_MAX_PAGE <= 7
		 and 7
    ELSE
		 and 0Fh ; Старшие биты выполняют спец функции
		 cp 9
		 jp c, correct
		  add a, 'A'-9-'1'
correct:
    ENDIF
		 add a, '1'
    IF NICE
		 ld (p_pageNum), a
    ELSE
		 inc de
		 ld (de), a
    ENDIF
		pop hl

dlg_redraw:
	        ; Выводим панель
		call	printStringEx

		DB C_INVERSE, C_BOTTOMLINE
		DB " F1-COPY F2-DELETE F3-MLOAD F4-MSAVE F5-EDIT F6-DOS F7-DEBUG"
		DB C_NORMAL
    IF NICE
		DB 0B0h ; Контрастно
    ELSE
		DB C_CLEARSCREEN ; очистка экрана
    ENDIF
    IF NICE
		DB 1Bh, 'Y', 20h+2, 20h+8 ; положение панели
    ELSE
		DB 1Bh, 'Y', 20h+2, 20h+7 ; положение панели
    ENDIF
		DB 17h, 93h, 8Bh
		DB 'R'
a_panelOA:	DB "AM"
    IF NICE
		DB '\'
p_pageNum:	DB "0 "
a_panelFilter:	DB "          "
    ELSE
		DB "_V6.0"
		DB 92
a_panelFilter:	DB 92,92,92
		DB '\'
p_pageNum:	DB "0  "
    ENDIF

    IF NICE
		DB 0C4h, 0C1h,	0D4h, 0C1h ; дата
		DB ' ', ' ', ' ', ' ', ' ', ' '
		DB 0C1h, 0C4h, 0D2h, 0C5h, 0D3h ; "адрес"
		DB ' ', ' ', ' '
		DB 0D3h, 0D5h, 0CDh, 0CDh, 0C1h ; "сумма"
		DB ' '
		DB 0D2h, 0C1h, 0DAh, 0CDh, 0C5h, 0D2h ; "размер"
		DB ' '
    ELSE
		DB 0C4h, 0C1h,	0D4h, 0C1h ; дата
		DB ' ', ' ', ' ', ' '
		DB 0C1h, 0C4h, 0CEh ; "адн"
		DB ' '
		DB 2Ch
		DB 0C1h, 0C4h, 0CBh ; "адк"
		DB ' '
		DB 2Dh
		DB 0D3h, 0D5h, 0CDh, 0CDh, 0C1h ; "сумма"
		DB ' '
		DB ' '
		DB 0C4h, 0CCh, 0C9h, 0CEh, 0C1h ; "длина"
		DB ' '
    ENDIF

		DB 8Ch ; темный фон
    IF NICE
		DB 0B8h ; Серый фон
    ENDIF
a_rightEdge:	DB 94h, 1Ah, 8, BORDER_L, 93h, 0 ; восстановление курсора экрана, вниз, влево, |, запоминание курсора экрана
    IF NO_MXDOS == 0
		ld (IO_FLOPPY_HEAD), a
    ENDIF	
pushCursorLine2: ; Сохранить курсор строки
		ld a, 8Dh
		jp j_printCharA

; ---------------------------------------------------------------------------

dlg_printSep:	call	printStringEx
		DB C_NORMAL, ' ', BORDER_H
    IF NICE == 0
		DB ' '
    ENDIF
		DB 0
		ret

; ---------------------------------------------------------------------------

    INCLUDE "pageReadBlock.inc"

; ---------------------------------------------------------------------------

dlg_checkMsg:	 ; Выйти с флагом Z, если выбрана последняя строка
		push de
		call	dlg_isLastLine
		pop de
		ret z

dlg_msg:	 ; Вывод сообщения
		push af
		 call	printStringEx
		 DB C_POPCURSORS, C_LEFT, C_INVERSE, ' ', ' ', ' ', 0C6h, 0C1h, 0CAh, 0CCh ; файл тип
		 DB ' ', ' ', 2Eh, 0D4h, 0C9h, 0D0h, ' ', 0Eh, ' ', 0
		 
		 ex de, hl
		 call	j_printString

		 call	printStringEx
aa:	DB ":?"
		 DB C_LEFT, 0
		pop af
		ret

; ---------------------------------------------------------------------------

dlg_isLastLine:	ld hl, (v_pageFreePtr)
		ex de, hl
		ld hl, (v_curFile)
		jp j_cmp_hl_de

; ---------------------------------------------------------------------------

    INCLUDE "fileGetStartAndSize2.inc" ; fileGetStartAndSize2 продолжается на sub_hl_de
    INCLUDE "sub_hl_de.inc"

; ---------------------------------------------------------------------------

dlg_f2:		 ; Вывод текста "удалить"
		ld de, a_delete
		call	dlg_checkMsg
		jp z, dlg_f2_alt

		; Пользователь должен нажать Enter
		call	dlg_yesNo

		; Удалить файл
		call	fileDelete

		; Сдвинуть экран вверх
    IF NICE == 0
		call	printStringEx
		DB C_NORMAL, C_POPCURSORL, 19h, C_SCROLLUPEX, 0
    ENDIF
		; Перерисовать экран
		ret

; ---------------------------------------------------------------------------

    INCLUDE "compareFileName.inc"
    INCLUDE "fileDelete.inc"
    INCLUDE "fileFromTapeCom.inc"

; ---------------------------------------------------------------------------

dlg_f3:		 ; Выводим "сравнить"
		ld de, a_verify
		call	dlg_checkMsg

		; Если файл не выбран
		jp z, dlg_f3_alt

		; ?
		call	dlg_yesNo

		; Продолжение в fileVerify

; ---------------------------------------------------------------------------

    INCLUDE "fileVerify.inc"

; ---------------------------------------------------------------------------

dlg_f3_alt:	 ; Вывод на экран строки "Чтение"
		ld de, a_load
		call	dlg_msgYesNo

    INCLUDE "fileFromTape.inc" ; fileFromTape продолжается

dlg_error:	 ; Выводим сообщение "ОШИБКА" + звуковой сигнал
		call	printStringEx
		DB 0CFh, 0DBh,	0C9h, 0C2h, 0CBh, 0C1h,	C_BEEP, 0 ; ошибка

waitEsc:	 ; Ждем, пока пользователь не нажмет ESC
		 call	keyWaitNoCursor
    IF NICE
		 cp ' '
    ELSE
		 cp 1Fh
    ENDIF
		jp nz, waitEsc
		; Выходим с установленным флагом C
		scf
		ret

; ---------------------------------------------------------------------------
; Вывод сообщения МАЛ ДИСК

outOfSpace:	call	printStringEx
		DB 0CDh, 0C1h,	0CCh, 20h, 0C4h, 0C9h, 0D3h, 0CBh, 0 ; мал диск
		jp waitEsc

; ---------------------------------------------------------------------------

    INCLUDE "printStringEx.inc"

; ---------------------------------------------------------------------------
; Сохранить файл на магнитофон

dlg_f4:		 ; Если файл не выбран, выходим
		ld hl, (f_nextFile)
		call	j_cmp_hl_de
		ret z

		; Вывод "запись"
		ld de, a_save
		call	dlg_msgYesNo

		call	dlg_isLastLine
		ld hl, (v_pageStart)
		jp z, fileToTape

		ld c, C_NORMAL

		ld hl, (v_curFile)
		ld de, 0Fh
		add hl, de
		call	pageWrite
		
		ld hl, (f_fileSize)
		ex de, hl
		ld hl, (v_curFile)

    INCLUDE "fileToTape.inc"

; ---------------------------------------------------------------------------
; Переименовать файл

dlg_f5:		 ; Вывести "имя:"
		ld de, a_name
		call	dlg_checkMsg

		; Если файл не выбран
		jp z, startEditor

dlg_f5_retry:	 ; Устанавливаем курсор на имя файла
		call	popCursorLine2

		; Редактирование имени файла
		ld hl, v_header_name
		ld de, 0CFFh
		call	j_input
	
		; Был нажат ESC	
		jp z, loc_F9D0

		; Была нажата служебная клавиша
		jp nc, dlg_f5_retry

		; Имя не введено, перерисовать
		ret p

		; Продолжение в fileSaveHeader

; ---------------------------------------------------------------------------
