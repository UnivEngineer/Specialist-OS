;----------------------------------------------------------------------------
; MXOS
; E.COM - текстовой редактор
;
; 2022-02-03 Дизассемблировано SpaceEngineer
;----------------------------------------------------------------------------

    INCLUDE "../include/mxos.inc"

; Компиляция оригинального редаткора (если 1)
STANDARD_EDITOR = 0

; Используемые подпрограммы DOS.SYS:
; bios_beep_Old         = 0C170h
; bios_delay_b          = 0C190h
; bios_cmp_hl_de        = 0C427h
; bios_memcpy_bc_hl     = 0C42Dh
; bios_reboot           = 0C800h
; bios_getch            = 0C803h
; bios_tapeRead         = 0C806h
; bios_printChar        = 0C809h
; bios_tapeWrite        = 0C80Ch
; bios_printString      = 0C818h
; bios_calcCS           = 0C82Ah
; bios_fileCreate       = 0C845h
; bios_fileLoadInfo     = 0C851h
; bios_fileNamePrepare  = 0C85Ah
; bios_fileLoad2        = 0C866h

; Используемые переменные DOS.SYS:
; bios_vars.lastKey     = 08FF0h
; bios_vars.tapeInverse = 08FF3h
; bios_vars.cursorDelay = 08FF4h
; bios_vars.inverse     = 08FFAh
; bios_vars.cursorY     = 08FFCh

; Используемые порты устройств:
; IO_KEYB_A = 0FFE0h
; IO_KEYB_B = 0FFE1h

;----------------------------------------------------------------------------

; Цвета

COLOR_TEXT      = 0B1h
COLOR_STATUSBAR = 071h

; Когда цвет включен, макрос вставляет такой код
    MACRO COLOR x
        IF STANDARD_EDITOR==0
            ld a, x
            ld (IO_COLOR), a
        ENDIF
    ENDM

;----------------------------------------------------------------------------

; Собственные переменные
    STRUCT EDITOR_VARIABLES
buffer  BLOCK  10, 0
last    BLOCK  1,  0
    ENDS

; История имени и экземпляр переменных размещены после кода ниже 1800h.


;----------------------------------------------------------------------------

; Начало программы
    IF NEW_MEMORY_MAP && !STANDARD_EDITOR
        ORG 0
    ELSE
        ORG 0D000h
    ENDIF

;----------------------------------------------------------------------------

; Буферы в конце программы
; Текст начинается с 1800h; фактический верхний предел берётся из memtop.

EDITOR_BUFFER_SIZE = APP_MEMORY_END - 01800h

;----------------------------------------------------------------------------

    INCLUDE "startup.inc" ; Старт, данные сеанса и цикл строки состояния
    INCLUDE "keyboard.inc" ; Диспетчер клавиш и таблицы Esc-команд
    INCLUDE "edit.inc" ; Редактирование строки и изменение размера текста
    INCLUDE "blocks.inc" ; Табуляция, страницы и выделение строк
    INCLUDE "tape.inc" ; Исторический магнитофонный формат и проверка записи
    INCLUDE "search.inc" ; Поиск, разбиение и объединение строк, копирование блоков
    INCLUDE "screen.inc" ; Примитивы памяти, курсор, экран и строка состояния
    INCLUDE "files.inc" ; Дисковые операции, ввод имени и дескриптор файла
    INCLUDE "entry.inc" ; Файловые клавиши и вход с аргументом командной строки

FILE_NAME_HISTORY: DS MAX_PATH,0
FILE_NAME_RECALL_END:
editor_vars EDITOR_VARIABLES

    IF !STANDARD_EDITOR
        ASSERT $ <= 01800h
    ENDIF
