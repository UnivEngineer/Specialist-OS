    MACRO ORG_PAD0 adr
        ASSERT $ <= adr
        BLOCK adr-$, 0
        ORG adr
    ENDM

; ===========================================================================
; Настройки

    IFNDEF MXOS_KERNEL
MXOS_KERNEL = 1
    ENDIF

STD_RAMFOS = 0				 ; Собирать исходный вариант RAMFOS

    IF STD_RAMFOS == 0

DATE_IN_RAM			= 1			 ; 0			Хранить дату в основном ОЗУ
RKS_SHIFT_LOADER	= 1			 ; 0			Запускать RKS в режиме MX при нажатом SHIFT
NO_ROM_ENTRY		= 1			 ; 0			Системные программы размещены как файлы
NO_PRINTER			= 1			 ; 0 		Отключить принтер
NO_MXDOS			= 1			 ; 0 		Отключить загрузку MXDOS с дискеты
OPEN_ANY_FILE		= 1			 ; 0 		Открывать редактором файлы с любым расширением
ALL_CHARS			= 1			 ; 0			Печатать все символы шрифта через ESC
RKS_LOADER			= 1			 ; 0			Запускать RKS
BIG_ROM				= 1			 ; 0			Использовать все 64 КБ ПЗУ MX2 как ROM-диск
WORK_WITHOUT_ARAM	= 0			 ; 32			Размер буфера в основном ОЗУ при отсутствии RAM-диска
ROM_PAGE_START		= 01880h	 ; 04000h	Первый байт страницы ПЗУ
ROM_PAGE_END		= 0FFFCh	 ; 0C000h	Конец страницы ПЗУ плюс один
CURSOR_BLINK_SPEED	= 767		 ; 767		Задержка мигания курсора
ARAM_MAX_PAGE		= 0Fh		 ; 6			Максимальный номер страницы RAM-диска
ARAM_PAGE_END		= 0FFBBh	 ; 0FFBBh	Конец страницы RAM-диска, далее общее ОЗУ
DLG_START_PAGE		= 80h		 ; 0			Начальная страница
INIT_SCREEN_CUR_COL	= 1			 ; 1			Очищать экран текущим цветом
INIT_SCREEN_COLOR	= 0F1h		 ; 0F1h		Цвет очистки экрана
FAST_PRINT			= 1			 ; 0			Быстрый вывод шрифта с непрерывными линиями и сохранением цвета справа
TRUE_CHECK_ARAM		= 1			 ; 0			Исправленное определение объёма RAM-диска
NICE				= 1			 ; 0			Новый интерфейс
COLOR_SUPPORT		= 1			 ; 0			Задавать стандартные цвета панелей и экрана
NO_PRINT_STATUS_BUG	= 1			 ; 0			Исправление ошибки
DLG_CURSOR_AT_TOP	= 0			 ; 0			Курсор на первом файле
NO_ROM_SWITCH_BUG	= 1			 ; 0			Исправление выбора ненулевой страницы ПЗУ
RELOCATE_FILE_FUNCTIONS = 1		 ; 0			Перенести часть функций из F800h в C800h для экономии места
SIZE_OPTIMIZATION	= 1			 ; 0			Экономия размера кода

    ELSE

DATE_IN_RAM			= 0
RKS_SHIFT_LOADER	= 0
NO_ROM_ENTRY		= 0
NO_PRINTER			= 0
NO_MXDOS			= 0
OPEN_ANY_FILE		= 0
ALL_CHARS			= 0
RKS_LOADER			= 0
BIG_ROM				= 0
WORK_WITHOUT_ARAM	= 0
ROM_PAGE_START		= 04000h
ROM_PAGE_END		= 0C000h
CURSOR_BLINK_SPEED	= 767
ARAM_MAX_PAGE		= 6
ARAM_PAGE_END		= 0FFBBh
DLG_START_PAGE		= 0
INIT_SCREEN_CUR_COL	= 1
INIT_SCREEN_COLOR	= 0F0h
FAST_PRINT			= 0
TRUE_CHECK_ARAM		= 0
NICE				= 0
COLOR_SUPPORT		= 0
NO_PRINT_STATUS_BUG	= 0
DLG_CURSOR_AT_TOP	= 0
NO_ROM_SWITCH_BUG	= 0
RELOCATE_FILE_FUNCTIONS = 0
SIZE_OPTIMIZATION	= 0

    ENDIF

; ===========================================================================
; Порты

IO_KEYB_A			= 0FFE0h
IO_KEYB_B			= 0FFE1h
IO_KEYB_C			= 0FFE2h
IO_KEYB_MODE		= 0FFE3h
IO_EXT_A			= 0FFE4h
IO_EXT_B			= 0FFE5h
IO_EXT_C			= 0FFE6h
IO_EXT_MODE			= 0FFE7h
IO_FLOPPY_CMD		= 0FFE8h
IO_FLOPPY_SECTOR	= 0FFEAh
IO_FLOPPY_DATA		= 0FFEBh
IO_TIMER			= 0FFECh
IO_FLOPPY_TRIG		= 0FFF0h
IO_FLOPPY_HEAD		= 0FFF2h
IO_COLOR			= 0FFF8h
IO_PAGE_RAM			= 0FFFCh
IO_PAGE_ARAM		= 0FFFDh
IO_PAGE_ROM			= 0FFFEh
IO_PAGE_STD			= 0FFFFh

; ===========================================================================
; Коды специальных символов

C_BEEP			= 7
C_LEFT			= 8
C_CLEARSCREEN	= 1Fh
C_PUSHCOLOR		= 88h
C_POPCOLOR		= 89h
C_INVERSE		= 8Bh
C_NORMAL		= 8Ch
C_PUSHCURSORL	= 8Dh
C_TOPLINE		= 8Fh
C_POPCURSORL	= 8Eh
C_BOTTOMLINE	= 90h
C_PUSHCURSORS	= 93h
C_POPCURSORS	= 94h
C_SCROLLUPEX	= 9Ah

; ===========================================================================
; Фиксированные константы

CHAR_HEIGHT	  		= 8
LINE_HEIGHT	 		= 10
SCREEN_HEIGHT	 	= 250
FILE_HEADER_SIZE	= 30
STACK_TOP			= 0D2FFh
RAM_TOP				= 8FFFh

; ===========================================================================
; Переменные

    ORG 0C000h

charGen:

    INCLUDE "chargen.inc"

keybMap:	DB 81h, 0ch, 19h, 1ah, 20h, 20h, 20h, 08h, 09h, 18h, 0ah, 0dh ; Нижний ряд клавиатуры
v_charGenPtr:	DW charGen
v_keybMap:	DW keybMap
					DB 51h, 5Eh, 53h, 4Dh,	49h, 54h, 58h, 42h, 40h, 2Ch, 2Fh, 5Fh ; Q^SMITXB@,/_
v_aramPageEnd:	DW ARAM_PAGE_END
v_aramPageStart:	DW 0
					DB 46h, 59h, 57h, 41h,	50h, 52h, 4Fh, 4Ch, 44h, 56h, 5Ch, 2Eh ; FYWAPROLDV\.
v_koi8:	DB 0 ; KOI-7=FF / KOI-8=00
v_rusLat:	DB 0 ; RUS=FF / LAT=00
v_capsLock:	DB 0 ; BP=00 / HP=02
v_printerEcho:	DB 0
					DB 4Ah, 43h, 55h, 4Bh, 45h, 4Eh, 47h, 5Bh, 5Dh, 5Ah, 48h, 3Ah ; JCUKENG[]ZH:
v_selColor:	DB 0B1h
v_aramMaxPage:	DB 0 ; Номер последней страницы RAM-диска
v_cursorPos:	DW 0 ; Координаты курсора в пикселях
					DB 3Bh, 31h, 32h, 33h,	34h, 35h, 36h, 37h, 38h, 39h, 30h, 2Dh ; ;1234567890-
v_beepConfig:	DW 0A05h
v_oldBeep:	DW 0
					DB 1Bh, 8Ah, 0, 1, 2, 3, 4, 5,	6, 8Bh,	8Ch, 1Fh ; Верхний ряд клавиатуры
v_inverse:	DB 0
					DB 0
v_aramMaxPageI:	DB ARAM_MAX_PAGE
v_tapeMode:	DB '0'
v_tapePulseB:	DW 322Ch ; 1200 бод
v_tapePulseA:	DW 1812h
v_tapePulseB2:	DW 211Bh ; 1800 бод
v_tapePulseA2:	DW 0F09h
v_tapePulseB3:	DW 1812h ; 2400 бод
v_tapePulseA3:	DW 0B05h
v_tapePresets:

; ===========================================================================
; Код

    ORG_PAD0 0C66Ch

    INCLUDE "tapeReadPilot.inc"
    INCLUDE "tapeWait.inc"
    INCLUDE "tapeRead.inc"
    INCLUDE "tapeWritePilot.inc"
    INCLUDE "tapeWrite.inc"
    INCLUDE "tapePulse.inc"
    INCLUDE "strToHex.inc"
    INCLUDE "calcCrc.inc"
    INCLUDE "pushPopCursor.inc"
    INCLUDE "setGetMemTop.inc"
    INCLUDE "pushColor.inc"

    IF $ > 0C800h
    ASSERT 0
    ENDIF
    ORG_PAD0 0C800h

    INCLUDE "c800.inc"
    INCLUDE "cmp_hl_de.inc"
    INCLUDE "inputFileName.inc"
    INCLUDE "printFileName.inc"
    INCLUDE "printCharA.inc"
    INCLUDE "biosInit.inc"
    INCLUDE "printString.inc"
    INCLUDE "parseDate.inc"
    IF TRUE_CHECK_ARAM
    INCLUDE "checkARAM_v2.inc"
    ELSE
    INCLUDE "checkARAM.inc"
    ENDIF
    INCLUDE "inputDate.inc"
    INCLUDE "topLine.inc" ; Продолжение в inverse
    INCLUDE "inverse.inc"
    INCLUDE "bottomLine.inc"
    INCLUDE "input.inc"
    INCLUDE "getCharFromScreen.inc"
    INCLUDE "memcmp_hl1_de_c.inc"
    INCLUDE "hl_div_2.inc"
    INCLUDE "fullClearScreen.inc"
    INCLUDE "printHex.inc"
    IF FAST_PRINT
    INCLUDE "printChar_fast.inc"
    ELSE
    INCLUDE "printChar.inc"
    ENDIF
    INCLUDE "calcCharAddr.inc"
    INCLUDE "printChar2.inc"
    IF FAST_PRINT != 1
    INCLUDE "printChar3.inc" ; Продолжение в setCursorPos
    ENDIF
    INCLUDE "setCursorPos.inc"
    INCLUDE "getCursorPos.inc"
    INCLUDE "scrollUp.inc"
    INCLUDE "memcpy_hl_de_c2.inc"
    INCLUDE "cursor.inc"
    INCLUDE "scrollDown.inc"
    INCLUDE "memcpyb_hl_de_c2.inc"
    INCLUDE "clearScreen.inc"
    INCLUDE "clearLine.inc"
    INCLUDE "keyWait.inc"
    INCLUDE "keyCursor.inc"
    INCLUDE "keyCheck.inc"
    INCLUDE "keyScan.inc" ; Продолжение в keySound
    INCLUDE "keySound.inc"
    INCLUDE "oldbeep.inc"
    INCLUDE "delay_l.inc"
    INCLUDE "beep.inc"
    INCLUDE "capsLock.inc"
    INCLUDE "tapeMode.inc"
    INCLUDE "printerEchoMode.inc"
    INCLUDE "printerHex.inc"
    INCLUDE "printerEcho.inc"
    INCLUDE "printer.inc"
    INCLUDE "setColorChar.inc"
    INCLUDE "setColor.inc"
    INCLUDE "popColor.inc"
    INCLUDE "bigRom.inc"
    IF RELOCATE_FILE_FUNCTIONS
    INCLUDE "fileSaveHeader.inc" ; Продолжение в pageWriteBlock
    INCLUDE "pageWriteBlock.inc"
    INCLUDE "fileSetType.inc"
    ENDIF

    INCLUDE "v_printerXlat.inc"

; При отсутствии RAM-диска используется этот буфер

    IF WORK_WITHOUT_ARAM != 0
miniPage:
    ORG_PAD0 $ + FILE_HEADER_SIZE + WORK_WITHOUT_ARAM + 2 ; Сохранять файлы в буфер WORK_WITHOUT_ARAM
miniPageEnd:
    DB 8 ; Место для терминатора
    ENDIF

a_lat:	DB 04Ch, 041h, 054h, 0 ; "LAT"
a_rus:	DB 0F2h, 0F5h,	0F3h, 0 ; "RUS"

    IF NICE
a_inputFileName:	DB ' ', 0C6h, 0C1h, 0CAh, 0CCh, 3Ah, ' ', C_PUSHCURSORL, 0 ; "файл" и сохранение курсора строки
    ELSE
a_inputFileName:	DB 11h, 0C6h, 0C1h, 0CAh, 0CCh, 3Ah, C_PUSHCURSORL, C_BEEP, 0 ; "файл" и сохранение курсора строки
    ENDIF

    A_INPUTDATE
    A_INITSCREEN

; ===========================================================================
; Переменные

    IF MXOS_KERNEL
    INCLUDE "compat.inc"
    ENDIF

    IF $ > 0D1F0h
    ASSERT 0
    ENDIF
    ORG_PAD0 0D1F0h

v_tape:	DW 0

; Второй заголовок файла

v_header2:	DB 0D3h, 0D3h, 0D3h ; Сигнатура 0D3h, 0D3h, 0D3h обязательна; переменные отдельно не инициализируются
v_header2_name:	DB ' ',' ',' ',' ',' ',' ',' ',' ',' '
v_header2_ext:	DB ' ',' ',' '
v_header2_saved:	DB 8Bh ; Признак сохранения файла (8Bh, 8Ch)
v_header2_date:	DB 01h, 11h, 13h ; Дата
					DB 0, 0, 0, 0, 0 ; Резерв MXDOS
v_header2_start:	DW 0
v_header2_end:	DW 0
v_header2_crc:	DW 0

; Конец заголовка файла

v_dblClear:	DB 0
					DB 0,0,0,0,0,0,0,0,0
unk_D21A:	DB 0,0,0,0,0,0
v_pushCursorLn:	DW 0
v_pushCursor:	DW 0
v_escMode:	DB 0
v_key2:	DB 0
v_printerCrLf:	DB 0
v_memTop:	DW 0
v_key0:	DW 0
v_key1:	DW 0
v_oldSP:	DW 0
					DB 0
v_topLineHeight:	DW 0
v_screenHeight:	DB 0 ; Высота экрана
v_screenHeight1:	DB 0 ; Высота экрана минус 10
v_bottomLineId:	DB 0
v_color:	DB 0 ; Текущий цвет фона и символов
v_pushColor:	DW 0
					DB 0,0,0,0
v_aramPageEnd2:	DW 0
v_escRow:	DB 0
					DB 0
v_lastKey:	DB 0
					DB 0
					DB 0
v_pageStart:	DW 0 ; Начало текущей страницы
v_page:	DB 0 ; Текущая страница

					DB 0,0,0,0, 0,0,0,0, 0,0,0,0

; Заголовок файла
; Первые 15 байтов читает fileFromTapeCom
; Первые 30 байтов читает fileLoadHeader


v_header:	DB 0D3h, 0D3h, 0D3h ; Сигнатура файла 0D3h, 0D3h, 0D3h
v_header_name:	DB ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' '
v_header_spc:	DB ' '
v_header_ext:	DB ' ', ' ', ' '
v_header_saved:	DB 0
v_header_date:	DB 0, 0, 0
v_header_unused:	DB 0, 0, 0, 0, 0
v_header_start:	DW 0
v_header_stop:	DW 0
v_header_crc:	DW 0

; Конец заголовка файла

v_curFile:	DW 0
v_printedFile:	DW 0
f_nextFile:	DW 0
f_fileBodySize:	DW 0
f_fileSize:	DW 0
v_pageFreeTotal:	DW 0
v_pageFreePtr:	DW 0

					DB 0F0h, 77h, 23h, 1Bh, 7Ah, 0B3h, 0CAh, 95h, 0D2h, 0Ch, 0C2h, 78h, 0D2h, 22h, 0, 8Fh, 0EBh, 22h, 2, 8Fh, 0C3h, 52h, 0D2h

buffer:	DB 0,0,0,0,0,0,0,0,0 ; До 9 символов даты, последний контролирует переполнение

; ===========================================================================
; Код

    IF MXOS_KERNEL
    ORG_PAD0 0D300h
    ORG 0F800h
    ELSE
    ORG_PAD0 0F800h
    ENDIF

    INCLUDE "f800.inc"
    INCLUDE "loadMXDOS.inc"
    INCLUDE "fileFind.inc"
    INCLUDE "pageNext.inc"
    INCLUDE "fileFind1.inc"
    INCLUDE "dialog.inc" ; Продолжение в fileSaveHeader
    IF RELOCATE_FILE_FUNCTIONS
	jp fileSaveHeader
    ELSE
    INCLUDE "fileSaveHeader.inc" ; Продолжение в pageWriteBlock
    INCLUDE "pageWriteBlock.inc"
    ENDIF
    INCLUDE "fileSetGetName.inc"
    IF RELOCATE_FILE_FUNCTIONS == 0
    INCLUDE "fileSetType.inc"
    ENDIF
    IF NO_ROM_ENTRY | RKS_LOADER
    INCLUDE "exec.inc"
    ENDIF

; ===========================================================================
; Переменные

a_defHeader:	DB 0D3h, 0D3h,	0D3h ; Сигнатура файла, далее 12 пробелов
a_12spaces:	DB "            " ; Пустое имя
a_exe:	DB "EXE"
a_com:	DB "COM"

    IF NO_MXDOS

a_copy:	DB 0CBh, 0CFh,	0D0h, 0C9h, 0D2h, 0CFh,	0D7h
				DB 0C1h, 0D4h, 0D8h, 0 ; "копировать"
a_deleteDate:	DB 0C4h, 0C1h,	0D4h, 0D5h, 20h ; "удалить дату"
a_delete:	DB 0D5h, 0C4h,	0C1h, 0CCh, 0C9h, 0D4h,	0D8h, 0 ; "удалить"
a_load:	DB 0DEh, 0D4h,	0C5h, 0CEh, 0C9h, 0C5h,	0 ; "чтение"
a_save:	DB 0DAh, 0C1h,	0D0h, 0C9h, 0D3h, 0D8h,	0 ; "запись"
a_verify:	DB 0D3h, 0D2h,	0C1h, 0D7h, 0CEh, 0C9h,	0D4h, 0D8h, 0 ; "сравнить"
a_name:	DB 0C9h, 0CDh,	0D1h, 0 ; "имя"

    ELSE	 ; С загрузчиком MXDOS не хватает нескольких байтов

a_copy:	DB "CP"
				DB 0
a_deleteDate:	DB "DT "
a_delete:	DB "DEL"
				DB 0
a_load:	DB "RD"
				DB 0
a_save:	DB "WR"
				DB 0
a_verify:	DB "CMP"
				DB 0
a_name:	DB "NAME"
				DB 0

    ENDIF

; ===========================================================================
; Непереключаемое окно процессора

    INCLUDE "window.inc"


    IF MXOS_KERNEL
ramfosLoadDialog = compatCancel
ramfosSaveDialog = compatCancel
ramfosReadAny = compatReadFailure
ramfosWriteAny = compatFailure
ramfosSave = compatFailure
ramfosSetAddr = compatFailure
ramfosGetName = compatFailure
ramfosSetName = compatFailure
ramfosFind = compatFailure
ramfosLoad = compatFailure
ramfosDelete = compatFailure
ramfosExec = compatFailure
    ELSE
ramfosLoadDialog = dlg_loadFile
ramfosSaveDialog = fileSaveDlg
ramfosReadAny = pageReadAny
ramfosWriteAny = pageWriteAny
ramfosSave = fileSave
ramfosSetAddr = fileSetAddr
ramfosGetName = fileGetName
ramfosSetName = fileSetName
ramfosFind = fileFind
ramfosLoad = fileLoad
ramfosDelete = fileDelete
ramfosExec = exec
    ENDIF

    ASSERT j_biosInit == 0C800h
    ASSERT j_pageReadAny == 0F836h
    ASSERT j_pageWriteAny == 0F839h
    ASSERT $ == 0FFE0h
    END
