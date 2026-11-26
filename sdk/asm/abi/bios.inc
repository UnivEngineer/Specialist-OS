; MXOS
; Прямые входы BIOS и блок переменных Монитора-2.

    IFNDEF MXOS_ABI_BIOS_INC
    DEFINE MXOS_ABI_BIOS_INC

bios_init = 0C000h  ; Инициализация из bootstrap / программный возврат в DOS

; Устаревшие точки, лучше не применять. Оставлены для совместимости.
bios_clearScreenOld = 0C010h  ; [OLD] Очистка экрана
bios_printCharOld   = 0C037h  ; [OLD] Вывод символа на экран
bios_beep_Old       = 0C170h  ; [OLD] Звуковой сигнал
bios_delay_l        = 0C18Fh  ; [OLD] Задержка l циклов
bios_delay_b        = 0C190h  ; [OLD] Задержка b циклов
bios_setKeybMode82  = 0C254h  ; [OLD] Загрузчик: A=82h, флаги/BC/DE/HL сохраняются
bios_setKeybMode91  = 0C25Ah  ; [OLD] Загрузчик: A=91h, флаги/BC/DE/HL сохраняются
bios_getchOld       = 0C337h  ; [OLD] Ожидание ввода с клавиатуры
bios_tapeReadOld    = 0C377h  ; [OLD] Чтение байта с магнитофона
bios_tapeWriteOld   = 0C3D0h  ; [OLD] Запись байта на магнитофон
bios_cmp_hl_de      = 0C427h  ; [OLD] Сравнить hl и de
bios_memcpy_bc_hl   = 0C42Dh  ; [OLD] HL..DE исключительно -> BC, с учётом перекрытия
bios_printStringOld = 0C438h  ; [OLD] Вывод строки на экран

bios_keybTable = 0C4A0h  ; [OLD] Матрица клавиатуры Загрузчика, 96 байт до C4FFh

; Стандартные точки C800h
bios_reboot       = 0C800h  ; Программный возврат в DOS и выбранную оболочку, без bootstrap
bios_getch        = 0C803h  ; Ожидание ввода с клавиатуры
bios_tapeRead     = 0C806h  ; Чтение байта с магнитофона
bios_printChar    = 0C809h  ; Вывод символа на экран
bios_tapeWrite    = 0C80Ch  ; Запись байта на магнитофон
bios_input        = 0C80Fh  ; Ввод строки с клавиатуры
bios_keyCheck     = 0C812h  ; Получить код нажатой клавиши, = bios_keyScan
bios_printHexByte = 0C815h  ; Вывод HEX числа на экран (байт)
bios_printStringZ = 0C818h  ; Вывод строки, HL=ASCII-Z; управляющие коды обрабатываются
bios_printString  = 0C818h  ; Вывод строки, HL=ASCII-Z; управляющие коды обрабатываются
bios_keyScan      = 0C81Bh  ; Получить код нажатой клавиши
bios_getCursorPos = 0C81Eh  ; Получить координаты курсора в hl (координаты в пикселях)
bios_tapeLoad     = 0C824h  ; Загрузить программу с магнитофона
bios_tapeSave     = 0C827h  ; Сохранить программу на магнитофон
bios_calcCS       = 0C82Ah  ; Расчет контрольной суммы
bios_setMemTop    = 0C830h  ; Monitor-2: установить границу из HL; регистры/флаги сохранены
bios_getMemTop    = 0C833h  ; Monitor-2: HL=граница памяти; AF/BC/DE и флаги сохранены

; Расширения MXOS следуют подряд за последним вектором Монитора-2.
bios_drawCursor           = 0C836h  ; Нарисовать или стереть курсор
bios_setCursorPos         = 0C839h  ; Установить координаты курсора из hl (координаты в пикселях)
bios_printHexWord         = 0C83Ch  ; Вывод HEX числа на экран (слово)
bios_printer              = 0C83Fh  ; Напечатать байт на принтере
bios_strToHex             = 0C842h  ; Преобразовать строку HEX в число
bios_installDriver        = 0C845h  ; Установить драйвер накопителя
bios_diskDriver           = 0C848h  ; Драйвер выбранного диска
bios_printCharRaw         = 0C84Bh  ; A=код 00h-FFh; AF/BC/DE/HL сохранены
bios_printDecWord         = 0C84Eh  ; Вывод числа экран в десятичной форме
bios_upperCase            = 0C851h  ; Перевод кода символа КОИ-8 в верхний регистр
bios_strcmp               = 0C854h  ; Сравнение строк, чувствительное к регистру
bios_stricmp              = 0C857h  ; Сравнение строк, нечувствительное к регистру
bios_sub_hl_de            = 0C85Ah  ; Вычитание hl и de: hl = hl - de
bios_fontControl          = 0C85Dh  ; Обработка управляющего кода движком шрифта
bios_fontNewLine          = 0C860h  ; Переход движка шрифта на новую строку
bios_fontPosition         = 0C863h  ; Вычисление позиции курсора для движка шрифта
bios_beepTimer            = 0C866h  ; Звуковой сигнал через ВИ53, ABI RAMFOS beep
bios_diskWriteChecked     = 0C869h  ; Общий B=5: BC=адрес драйвера, DE=сектор, HL=данные
bios_setFontEncoding      = 0C86Ch  ; A=0 KOI8, иначе KOI7; перестановка шрифта/пререндера, регистры сохранены
bios_argNext              = 0C86Fh  ; HL=курсор -> HL=разделитель, DE=слово, B=длина; CF=конец, C сохранён
bios_installBlockDriver   = 0C872h  ; A=диск, HL=драйвер, C=log2 физического блока (7..9)
bios_setInputHook         = 0C875h  ; HL=callback/0, возврат HL=прежний; DE/AF разрушены
bios_blockIO              = 0C878h  ; A=диск, B=1/2, DE=физический блок, HL=буфер; BC/DE/HL сохранены
bios_printStringRawZ      = 0C87Bh  ; HL=ASCIIZ; 01h-FFh рисуются как глифы
bios_printStringRawLim    = 0C87Eh  ; HL=байты, BC=длина; AF/BC/DE сохранены, HL=конец
bios_printStringRawPseudo = 0C881h  ; HL=байты, BC=длина; RawLim с дорисовкой рамок KOI8-R
bios_printStringLim       = 0C884h  ; HL=байты, BC=длина; управляющие коды, AF/BC/DE сохранены, HL=конец
bios_cliExecDone          = 0C887h  ; [MXOS2] Возврат CLI после FILE_EXEC, CF/A — результат
bios_cliExecError         = 0C88Ah  ; Код отказа для следующего запуска CLI

    STRUCT BIOS_VARIABLES
keyboardHook    DW  -1      ; 8FDFh — обработчик клавиатуры Монитора-2
tapeError       DW  0C800h  ; 8FE1h - Адрес, куда происходит переход при ошибке чтения с ленты
tapeAddr        DW  -1      ; 8FE3h - Адрес программы, загруженной с ленты
inputHook       DW  -1      ; 8FE5h — обработчик строки Монитора-2
monitorFont     DW  018A0h  ; 8FE7h — исторический адрес шрифта / 8
currentChar     DB  0
cursorVisible   DB  0
inputBusy       DB  0
keyLocks        DB  0BAh    ; 3Ah      ; 8FECh - KOI8: bit0=RUS, bit7=нижний регистр без Shift; остальные биты сохранены
keyHandler      DW  -1      ; 8FEDh -
lastLastKey     DB  -1      ; 8FEFh - Предпоследняя нажатая клавиша
lastKey         DB  -1      ; 8FF0h - Последняя нажатая клавиша
beepFreq        DB  5Fh     ; 8FF1h - Частота звукового сигнала (также влияет на длительность)
beepDuration    DB  20h     ; 8FF2h - Длительность звукового сигнала (сколько раз повторять посылку)
tapeInverse     DB  0FFh    ; 8FF3h - Признак инверсии данных с ленты
tapeWriteCode0  DB  20h
tapeWriteCode1  DB  0E0h
oldSP           DW  -1      ; 8FF6h - Используется для сохранения SP некоторыми функциями
oldHL           DW  0       ; 8FF8h - Используется для сохранения HL функцией печати символа Монитора-2
inverse         DW  0       ; 8FFAh - Инвертирование текста (0=нормальный текст, 0FFFFh=инверсный)
cursorY         DB  -1      ; 8FFCh - Положение курсора по вертикали в пикселях
cursorX         DB  -1      ; 8FFDh - Положение курсора по горизонтали в пикселях / 2
writeDelay      DB  28h     ; 8FFEh - Скорость при записи на ленту
readDelay       DB  3Ch     ; 8FFFh - Скорость при чтении с ленты
    ENDS

; Блок переменных BIOS начинается с адреса 8FDFh
bios_vars   BIOS_VARIABLES = 8FDFh

    ASSERT BIOS_VARIABLES == 33
    ASSERT bios_vars + BIOS_VARIABLES == 09000h

; Состояние установленного шрифта и курсора в основной памяти MXOS.
; Этими адресами пользуются MAIN-реализации UI. Для koi7 разрешено чтение,
; кодировку меняет bios_setFontEncoding или MXOS_BIOS_SET_FONT_ENCODING.
bios_upper.charGen   = 0EF90h  ; физический адрес RAM-шрифта, слово
bios_upper.cursorCfg = 0EF92h  ; параметры курсора, байт
bios_upper.koi7      = 0EF93h  ; 0 = KOI8, FFh = KOI-7R

    ENDIF
