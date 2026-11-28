; MXOS
; Прямые входы BIOS и блок переменных Монитора-2.

    IFNDEF MXOS_ABI_BIOS_INC
    DEFINE MXOS_ABI_BIOS_INC

mon2_init            = 0C000h  ; Первичный вход bootstrap и программный возврат.
mon2_clearScreenOld  = 0C010h
mon2_printCharOld    = 0C037h
mon2_beep_Old        = 0C170h
mon2_delay_l         = 0C18Fh
mon2_delay_b         = 0C190h
mon2_setKeybMode82   = 0C254h
mon2_setKeybMode91   = 0C25Ah
mon2_getchOld        = 0C337h
mon2_tapeReadOld     = 0C377h
mon2_tapeWriteOld    = 0C3D0h
mon2_cmp_hl_de       = 0C427h
mon2_memcpy_bc_hl    = 0C42Dh
mon2_printStringOld  = 0C438h
mon2_keybTable       = 0C4A0h  ; Матрица клавиатуры, 96 байт до C4FFh.

; Векторы Монитора-2 в MAIN после выбора профиля BIOS_PROFILE_MON2.
mon2_reboot          = 0C800h
mon2_getch           = 0C803h
mon2_tapeRead        = 0C806h
mon2_printChar       = 0C809h
mon2_tapeWrite       = 0C80Ch
mon2_input           = 0C80Fh
mon2_keyCheck        = 0C812h
mon2_printHexByte    = 0C815h
mon2_printStringZ    = 0C818h
mon2_keyScan         = 0C81Bh
mon2_getCursorPos    = 0C81Eh  ; H = X/2, L = Y в пикселях.
mon2_tapeLoad        = 0C824h
mon2_tapeSave        = 0C827h
mon2_calcCS          = 0C82Ah
mon2_setMemTop       = 0C830h
mon2_getMemTop       = 0C833h

; Нативные векторы с историческим ABI RAMFOS в MAIN.
bios_init            = 0C800h  ; Инициализация клавиатуры, таймера и экрана.
bios_getch           = 0C803h
bios_tapeRead        = 0C806h
bios_printChar       = 0C809h  ; C = символ.
bios_tapeWrite       = 0C80Ch
bios_printCharA      = 0C80Fh
bios_keyCheck        = 0C812h  ; FFh/NZ — готов символ, 0/Z — очередь пуста.
bios_printHexByte    = 0C815h
bios_printStringZ    = 0C818h
bios_printString     = bios_printStringZ
bios_keyScan         = 0C81Bh
bios_getCursorPos    = 0C81Eh  ; H = строка, L = колонка.
bios_getGlyph        = 0C821h
bios_tapeLoad        = 0C824h
bios_tapeSave        = 0C827h
bios_calcCS          = 0C82Ah
bios_bottomLine      = 0C82Dh
bios_getMemTop       = 0C830h
bios_setMemTop       = 0C833h
bios_input           = 0C836h
bios_strToHex        = 0C839h
bios_setCursorPos    = 0C83Ch  ; H = строка, L = колонка.
bios_printerEcho     = 0C83Fh
bios_printer         = 0C842h
bios_fileDialog      = 0C845h
bios_beepTimer       = 0C848h
bios_setColor        = 0C84Bh
bios_clearScreen     = 0C84Eh
bios_oldBeep         = 0C851h
bios_cmp_hl_de       = 0C854h
bios_printerInit     = 0C857h
bios_cycleTapeMode   = 0C85Ah
bios_reboot          = 0F800h  ; Программный возврат из RAMFOS/MX.

BIOS_PROFILE_RAMFOS  = 1
BIOS_PROFILE_MON2    = 2
BIOS_PROFILE_DEFAULT = BIOS_PROFILE_RAMFOS
BIOS_PROFILE_QUERY   = 0FFh

BIOS_BEEP_FREQ_DEFAULT     = 5Fh
BIOS_BEEP_DURATION_DEFAULT = 20h

    STRUCT MON2_VARIABLES
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
beepFreq        DB  BIOS_BEEP_FREQ_DEFAULT     ; 8FF1h - Частота звукового сигнала (также влияет на длительность)
beepDuration    DB  BIOS_BEEP_DURATION_DEFAULT     ; 8FF2h - Длительность звукового сигнала (сколько раз повторять посылку)
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
mon2_vars   MON2_VARIABLES = 8FDFh

    ASSERT MON2_VARIABLES == 33
    ASSERT mon2_vars + MON2_VARIABLES == 09000h

; Состояние установленного шрифта и курсора в основной памяти MXOS.
; Этими адресами пользуются MAIN-реализации UI. Для koi7 разрешено чтение,
; кодировку меняет bios_setFontEncoding или MXOS_BIOS_SET_FONT_ENCODING.
bios_upper.charGen   = 0EF90h  ; физический адрес RAM-шрифта, слово
bios_upper.cursorCfg = 0EF92h  ; параметры курсора, байт
bios_upper.koi7      = 0EF93h  ; 0 = KOI8, FFh = KOI-7R

    ENDIF
