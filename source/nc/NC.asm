;----------------------------------------------------------------------------
; MXOS - Файловый менеджер NC.COM
;
; 2013-12-18 Дизассемблировано и доработано vinxru
; 2022-01-31 Доработано SpaceEngineer
;----------------------------------------------------------------------------

    INCLUDE "../include/mxos.inc"
IMAGE_FLAGS = IMAGE_FLAG_MAIN
    INCLUDE "../include/ui.inc"

    ORG NC_LOAD_ADDR

; Список файлов на всю высоту панелей
    IFNDEF FULL_PANELS
FULL_PANELS = 0
    ENDIF

; 1: битмап при едином атрибуте файлов, 0: печать всех имён.
    IFNDEF NC_BITMAP_SCROLL
NC_BITMAP_SCROLL = 1
    ENDIF
    ASSERT NC_BITMAP_SCROLL == 0 || NC_BITMAP_SCROLL == 1

; Предустановки цветов:
; 0 = vinxru
; 1 = новый
; 2 = Norton Commander
COLOR_PRESET = 1

COLOR_CMDLINE    =  070h        ; Цвет командной строки
COLOR_CMDSCREEN  =  COLOR_BIOS  ; Цвет командного экрана (когда панели спрятаны)

    IF COLOR_PRESET == 0

COLOR_BORDER     =  0F1h    ; Цвет рамки
COLOR_PANELNAME  =  0A1h    ; Цвет заголовка панели (NAME)
COLOR_FILE       =  0B1h    ; Цвет файлов
COLOR_INFOLINE   =  0B1h    ; Цвет строки информации о текущем файле
COLOR_CURSOR     =  0B0h    ; Цвет выделенного файла (инверсный)
COLOR_DIALOG     =  007h    ; Цвет окон диалогов F1-F9
COLOR_DIALOG_ERR =  047h    ; Цвет текста на окне с сообщением об ошибке
COLOR_INFOHEADER =  0A1h    ; Заголовок информационной панели
COLOR_INFONUMBER =  0E1h    ; Цифры на информационной панели
COLOR_INFOTEXT   =  0F1h    ; Текст на информационной панели
COLOR_HELP_F     =  040h    ; Цвет функциональных клавиш в строке подсказки
COLOR_HELP_TEXT  =  071h    ; Цвет текста в строке подсказки

    ELSEIF COLOR_PRESET == 1

COLOR_BORDER     =  0F1h    ; Цвет рамки
COLOR_PANELNAME  =  0E1h    ; Цвет заголовка панели (NAME)
COLOR_FILE       =  0B1h    ; Цвет файлов
COLOR_INFOLINE   =  0B1h    ; Цвет строки информации о текущем файле
COLOR_CURSOR     =  030h    ; Цвет выделенного файла (инверсный)
COLOR_DIALOG     =  007h    ; Цвет окон диалогов F1-F9
COLOR_DIALOG_ERR =  047h    ; Цвет текста на окне с сообщением об ошибке
COLOR_INFOHEADER =  0E1h    ; Заголовок информационной панели
COLOR_INFONUMBER =  0E1h    ; Цифры на информационной панели
COLOR_INFOTEXT   =  0F1h    ; Текст на информационной панели
COLOR_HELP_F     =  060h    ; Цвет функциональных клавиш в строке подсказки
COLOR_HELP_TEXT  =  030h    ; Цвет текста в строке подсказки

    ELSE

COLOR_BORDER     =  0B1h    ; Цвет рамки
COLOR_PANELNAME  =  0E1h    ; Цвет заголовка панели (NAME)
COLOR_FILE       =  0B1h    ; Цвет файлов
COLOR_INFOLINE   =  0B1h    ; Цвет строки информации о текущем файле
COLOR_CURSOR     =  030h    ; Цвет выделенного файла (инверсный)
COLOR_DIALOG     =  007h    ; Цвет окон диалогов F1-F9
COLOR_DIALOG_ERR =  047h    ; Цвет текста на окне с сообщением об ошибке
COLOR_INFOHEADER =  0B1h    ; Заголовок информационной панели
COLOR_INFONUMBER =  0E1h    ; Цифры на информационной панели
COLOR_INFOTEXT   =  0B1h    ; Текст на информационной панели
COLOR_HELP_F     =  070h    ; Цвет функциональных клавиш в строке подсказки
COLOR_HELP_TEXT  =  030h    ; Цвет текста в строке подсказки

    ENDIF

;---------------------------------------------------------------------------
; Макросы
;---------------------------------------------------------------------------

; Когда цвет включен, макрос вставляет такой код
    MACRO COLOR x
        IF ENABLE_COLOR
            IF x == COLOR_INFOTEXT
                call ncColorText
            ELSEIF x == COLOR_INFONUMBER
                call ncColorNumber
            ELSEIF x == COLOR_CMDLINE
                call ncColorCommand
            ELSEIF x == COLOR_DIALOG
                call ncColorDialog
            ELSEIF x == COLOR_FILE
                call ncColorFile
            ELSE
                ld a, x
                ld (IO_COLOR), a
            ENDIF
        ENDIF
    ENDM

; Макрос для нахождения координат строки длиной W, отцентрированной горизонтально
    MACRO CENTER_LINE w, y
        ld  hl, (((60h - w * 3 / 2 + 2) & 0FCh) << 8) + y
        ld  (bios_vars.cursorY), hl
    ENDM

;---------------------------------------------------------------------------
; Код
;---------------------------------------------------------------------------

    INCLUDE "start.inc"     ; Продолжается в main
    INCLUDE "main.inc"
    INCLUDE "selFileToCmdLine.inc"
    INCLUDE "butF4.inc"
    INCLUDE "directories.inc"
    INCLUDE "dirty.inc"
    INCLUDE "butEnter.inc"
    INCLUDE "saveLoadState.inc"
    INCLUDE "drawWindow.inc"
    INCLUDE "printStringInv.inc"
    INCLUDE "inputForCopyMove.inc"
    INCLUDE "printSelDrive.inc"
    INCLUDE "butF7.inc"
    INCLUDE "butF6.inc"
    INCLUDE "butF5.inc"
    INCLUDE "copyFileInt.inc"
    INCLUDE "overwrite.inc"
    INCLUDE "../include/errors.inc"
    INCLUDE "printInvSelFile.inc"
    INCLUDE "butF8.inc"
    INCLUDE "butF2.inc"
    INCLUDE "butTab.inc"
    INCLUDE "butF3.inc"
    INCLUDE "butArrows.inc"
    INCLUDE "clearCmdLine.inc"    ; Продолжается в printSpaces
    INCLUDE "printSpaces.inc"
    INCLUDE "drawCursor.inc"
    INCLUDE "keepCursorForKey.inc"
    INCLUDE "printInfoLine.inc"
    DEFINE UI_IMPLEMENTATION
    INCLUDE "../include/ui.inc"
inverseRect = uiInverseRect
    INCLUDE "getSelectedFile.inc"
    INCLUDE "loadAndPrint.inc"
    INCLUDE "fullRedraw.inc"
    INCLUDE "loadFiles.inc"
    INCLUDE "catalogCache.inc"
    INCLUDE "sort.inc"
    INCLUDE "sortDialog.inc"
    INCLUDE "printInfoPanel.inc"
    INCLUDE "printFilePanel.inc"
    IF NC_BITMAP_SCROLL
    INCLUDE "scrollBitmap.inc"
    ENDIF
    INCLUDE "printCurDrive.inc"
    INCLUDE "rwBytePanel.inc"
    INCLUDE "printFileName.inc"
    INCLUDE "printString2.inc"
    INCLUDE "setCursorPosPanel.inc"
    INCLUDE "draw.inc"

    INCLUDE "compactName.inc"
    INCLUDE "input.inc"
    INCLUDE "shift.inc"
    INCLUDE "colors.inc"

    MODULE ncMath
    DEFINE MATH_DIV2
    INCLUDE "../include/math.inc"
    ENDMODULE

    INCLUDE "fontRender.inc"

;---------------------------------------------------------------------------
; Константы и переменные
;---------------------------------------------------------------------------

aF1LeftF2RighF3:
        UI_TOOLBAR_FIELD 2,0,"F1"
        UI_TOOLBAR_FIELD 4,080h,"Left"
        UI_TOOLBAR_FIELD 2,0,"F2"
        UI_TOOLBAR_FIELD 4,080h,"Rght"
        UI_TOOLBAR_FIELD 2,0,"F3"
        UI_TOOLBAR_FIELD 4,080h,"Info"
        UI_TOOLBAR_FIELD 2,0,"F4"
        UI_TOOLBAR_FIELD 4,080h,"Edit"
        UI_TOOLBAR_FIELD 2,0,"F5"
        UI_TOOLBAR_FIELD 4,080h,"Copy"
        UI_TOOLBAR_FIELD 2,0,"F6"
        UI_TOOLBAR_FIELD 4,080h,"Move"
        UI_TOOLBAR_FIELD 2,0,"F7"
        UI_TOOLBAR_FIELD 4,080h,"MkDr"
        UI_TOOLBAR_FIELD 2,0,"F8"
        UI_TOOLBAR_FIELD 4,080h,"Del"
        DB 0
aShiftToolbar:
        UI_TOOLBAR_FIELD 2,0,"F1"
        UI_TOOLBAR_FIELD 4,080h,"Sort"
        UI_TOOLBAR_FIELD 2,0,"F2"
        UI_TOOLBAR_FIELD 4,080h,"Sort"
        UI_TOOLBAR_FIELD 2,0,"F3"
        UI_TOOLBAR_FIELD 4,080h,"Info"
        UI_TOOLBAR_FIELD 2,0,"F4"
        UI_TOOLBAR_FIELD 4,080h,"New"
        UI_TOOLBAR_FIELD 2,0,"F5"
        UI_TOOLBAR_FIELD 4,080h,"Copy"
        UI_TOOLBAR_FIELD 2,0,"F6"
        UI_TOOLBAR_FIELD 4,080h,"Ren"
        UI_TOOLBAR_FIELD 2,0,"F7"
        UI_TOOLBAR_FIELD 3,080h,"MD"
        UI_TOOLBAR_FIELD 3,0,"F10"
        UI_TOOLBAR_FIELD 4,080h,"Exit"
        DB 0
aCommanderVer:      DB "Commander version 3.0",0
aCopyright:         DB "(C) Omsk 1992, SPb 2022",0
aFileProtected:     DB "System or read-only file",0
aChooseDrive:       DB "  Choose drive: ",0
aDeleteFrom:        DB "Delete from ",0
asc_DC17:           DB 8, ' ',8, 0

; Маркер пропущенных каталогов в заголовках и статусе, только для отображения.
; Например, '...' можно заменить на ' '. Длины вычисляются при сборке.
compactPathPrefix: DB ':',05Ch
pathOmissionMark:   DB '...'
PATH_OMISSION_LENGTH EQU $ - pathOmissionMark
                   DB 05Ch
PATH_PREFIX_LENGTH EQU $ - compactPathPrefix
PATH_RESERVED_LENGTH EQU 1 + PATH_PREFIX_LENGTH ; Ещё один знак — буква диска.
    ASSERT PATH_RESERVED_LENGTH + 12 <= 23     ; Полное имя FAT 8.3 в статусе.

aCopyFromTo:        DB "Copy from    to",8,8,8,8,8, 0
aCopyFailed:        DB "Copy failed",0
aFileAlreadyExists: DB "File already exists",0
aDeleteFailed:      DB "Delete failed",0
aCantRenameFile:    DB "Can",39,"t rename file!",0
aRenameOn:          DB "Rename on ",0
aKBytesExtMemory:   DB 18h,"KB extended memory",0   ; здесь и далее 18h вместо ведущего пробела,
aKBytesMemory:      DB 18h,"bytes Memory",0         ; чтобы не портить цвет предыдущего символа
aKBytesFree:        DB 18h,"bytes Free",0
aKBytesTotalOnDrv:  DB 18h,"KB total on drive ",0
aKBytesFreeOnDrv:   DB 18h,"KB free  on drive ",0
aFilesUse:          DB 18h,"files use ",0
aKBytesIn:          DB 18h,"KB",0
aVolumeLabel:       DB "Volume label: ",0
aDiskDriver:        DB "Driver: ",0
aDiskTechnology:    DB "Technology: ",0
aTypeROM:           DB "ROM",0
aTypeRAM:           DB "RAM",0
aTypeFlash:         DB "Flash",0
aDriverUnknown:     DB "n/a",0,0
aDrive:             DB "Drive ",0
aNotFormatted:      DB 18h,"is not formatted",0


aEditor:            DB "A:",05Ch,"E.COM",0Dh      ; терминатором тут должно быть 0Dh

;-----------------------------------------------------------------------
; Описания окон


g_filePanel:
            G_WINDOW 0, 0, 4, 192, 226        ; 2, 0, 90h, 0E0h, 16h
g_fileDividers:
            G_LINE   0, 4, 208, 184           ; 1, 0D0h, 90h, 16h, 0Fh, 0F0h
            G_VLINE  0, 96, 7, 201            ; 3, ?, 9Ch, 0CDh, 80h
            DB 0

g_infoPanel:
            G_WINDOW 0, 0, 4, 192, 226        ; 2, 0, 90h, 0E0h, 16h
g_infoDividers:
            G_LINE   0, 4, 31, 184            ; 1, 1Fh, 90h, 16h, 0Fh, 0F0h
            G_LINE   0, 4, 136, 184           ; 1, 70h, 90h, 16h, 0Fh, 0F0h
            DB 0

g_chooseDrive:
            G_WINDOW 0, 40, 85, 112, 50       ; 2, 55h, 95h, 02Ch, 0Ch
            G_LINE   0, 44, 103, 104          ; 1, 67h, 95h, 0Ch, 0Fh, 0F0h
            DB 0

g_window1:
            G_WINDOW 80h, 112, 80, 160, 37    ; 82h, 50h, 9Eh, 1Fh, 12h
            G_LINE   80h, 116, 98, 152        ; 81h, 62h, 9Eh, 12h, 0Fh, 0F0h
            DB 0

g_window2:
            G_WINDOW 80h, 104, 114, 176, 37   ; 82h, 72h, 9Dh, 1Fh, 14h
            G_LINE   80h, 108, 132, 168       ; 81h, 84h, 9Dh, 14h, 0Fh, 0F0h
            DB 0

;-----------------------------------------------------------------------
; Размер буфера для листинга директории (штук дескрипторов файлов)

    IF FULL_PANELS
FILE_LIST_SIZE    = 40
    ELSE
FILE_LIST_SIZE    = 38
    ENDIF

;-----------------------------------------------------------------------
; Коодинаты элементов панелей от левого верхнего угла экрана
; x - пиксели / 2, y - пиксели; X кратен 4 для пакетной печати.

P_FILE_LIST_X1    = 4   ; Файловая таблица (2 колонки)
P_FILE_LIST_X2    = 52
    ASSERT (P_FILE_LIST_X1 & 3) == 0 && (P_FILE_LIST_X2 & 3) == 0
    ASSERT P_FILE_LIST_X1 * 2 + 12 * 6 <= 88
    ASSERT P_FILE_LIST_X2 * 2 + 12 * 6 <= 184

    IF FULL_PANELS
P_FILE_LIST_Y     = 12
    ELSE
P_FILE_LIST_Y     = 22
    ENDIF

P_FILE_LIST_H     = FILE_LIST_SIZE / 2  ; Высота файловой таблицы в строках
P_FILE_LIST_Y_MAX = P_FILE_LIST_H * 10 + P_FILE_LIST_Y

; Геометрия общей очистки хвостов и необязательной битмап-прокрутки.
SCROLL_TOP EQU P_FILE_LIST_Y - 8
SCROLL_BOTTOM EQU P_FILE_LIST_Y_MAX - 8
SCROLL_HEIGHT EQU P_FILE_LIST_H * 10
SCROLL_LEFT EQU 90h + P_FILE_LIST_X1 * 2 / 8
SCROLL_RIGHT EQU 90h + P_FILE_LIST_X2 * 2 / 8

P_DRIVE_LETTER_X  = 9   ; Буква имени диска внизу панели
P_DRIVE_LETTER_Y  = 222
P_FILE_NAME_X     = 4  ; Имя файла внизу панели
P_FILE_NAME_Y     = 222
P_FILE_DATA_X     = 68  ; Данные о файле (адрес загрузки, размер)
P_FILE_DATA_Y     = 222

P_INPUT_WIDTH     = 24  ; Длина поля ввода имени файла в символах

;-----------------------------------------------------------------------
; Переменные
;-----------------------------------------------------------------------

; Блок переменных состояния
; Сохраняется в B:\NCSTATE.TMP; не занимает состояние ядра DOS.

    STRUCT NC_STATE
initState           DB    5Bh   ; признак, что NC.COM уже запускался
activePanel         DB    0     ; номер активной панели
panelA_dirLow       DB    0
panelB_dirLow       DB    0
panelA_drive        DB    0     ; номер диска панели A
panelB_drive        DB    7     ; номер диска панели B
panelA_dirHigh      DB    0
panelB_dirHigh      DB    0
panelA_firstFile    DB    0     ; c какого файла начинается панель A
panelB_firstFile    DB    0     ; c какого файла начинается панель B
panelA_curFile      DB    0     ; на каком файле курсор на панели A
panelB_curFile      DB    0     ; на каком файле курсор на панели B
panelA_sort         DB    2     ; 0=порядок диска, 1=имя/расширение, 2=расширение/имя
panelB_sort         DB    2
    ENDS

state   NC_STATE
stateEnd:           ; адрес конца блока переменных состояния

; Другие переменные - не включены в бинарник (машинный код не
; генерируется, при первом запуске переменные могут содержать мусор)

    STRUCT NC_VARIABLES
panelA_info     DB    0
panelB_info     DB    0
panelA_filesCnt DB    0
panelB_filesCnt DB    0
path           BLOCK MAX_PATH,0
pathA          BLOCK MAX_PATH,0 ; Полный путь левой панели.
pathB          BLOCK MAX_PATH,0 ; Полный путь правой панели.
pathValidA     DB 0
pathValidB     DB 0
pathDriveA     DB 0
pathDriveB     DB 0
pathLowA       DB 0
pathLowB       DB 0
pathHighA      DB 0
pathHighB      DB 0
drawnLeftA     DB 0             ; Занятые строки в первой колонке.
drawnLeftB     DB 0
drawnRightA    DB 0             ; Занятые строки во второй колонке.
drawnRightB    DB 0
newRows        DB 0             ; Временный размер видимого списка.
pathArg        BLOCK MAX_PATH,0
displayPath    BLOCK MAX_PATH,0
fileListPtr     DW    0         ; Видимое окно индексной таблицы активной панели.
cacheValidA     DB    0
cacheValidB     DB    0
cacheSlotA      DB    0FFh     ; Номер общего каталога; FF — ещё не назначен.
cacheSlotB      DB    0FFh
cacheSortA      DB    0
cacheSortB      DB    0
cmdLinePos      DW    0
cmdLineEnd      DW    0
cmdTextPos      DW    0         ; Начало видимой части команды после приглашения.
cmdCapacity     DB    0         ; Число видимых символов команды.
cmdOffset       DB    0         ; Смещение видимого окна в буфере команды.
chooseDrive     DB    0
copyRequest     FILE_COPY_REQUEST
scrollCursorKept DB   0         ; Старый курсор остаётся на краю при шаге 1.
scrollRows      DB    0         ; Модуль изменения начала списка (1..255).
scrollPixels    DB    0         ; Высота новых строк, не более одной колонки.
scrollRemain    DB    0         ; Высота сохраняемой части колонки.
diskInfoPtr     DW    0         ; адрес структуны DISK_INFO
driveCount     DB    0         ; число установленных дисковых драйверов
driveList      BLOCK 8, 0     ; номера дисков с установленным драйвером

input           BLOCK MAX_PATH, 0
    ENDS

vars    NC_VARIABLES = $        ; адрес начала блока переменнных
varsEnd = vars + NC_VARIABLES
state.panelA_info = vars.panelA_info
state.panelB_info = vars.panelB_info
state.panelA_filesCnt = vars.panelA_filesCnt
state.panelB_filesCnt = vars.panelB_filesCnt
vars.tempFileDescr = vars.copyRequest.destination

; Все рабочие буферы принадлежат NC и находятся после его кода.
FILE_CACHE_CAPACITY   = 255     ; Нынешние индексы NC — 8-разрядные.
FILE_CACHE_BYTES      = FILE_CACHE_CAPACITY * FILE_INFO_SIZE + 1
FILE_CACHE_A          = varsEnd
FILE_CACHE_B          = FILE_CACHE_A + FILE_CACHE_BYTES
FILE_CACHE_C          = FILE_CACHE_B + FILE_CACHE_BYTES
    ASSERT FILE_INFO_SIZE == 16

; Единая системная строка принадлежит DOS и не входит в область NC.
vars.cmdLine = CMD_LINE
vars.cmdLineCtrl = CMD_LINE + 127
FILE_INDEX_A = (FILE_CACHE_C + FILE_CACHE_BYTES + 255) & 0FF00h
FILE_INDEX_B = FILE_INDEX_A + FILE_CACHE_CAPACITY + 1
CATALOG_INDEX_A = FILE_INDEX_B + FILE_CACHE_CAPACITY + 1
NC_SORT_KEYS = (CATALOG_INDEX_A + 3 * (FILE_CACHE_CAPACITY + 1) + 255) & 0FF00h
NC_SORT_RANKS = NC_SORT_KEYS + 11 * 256
NC_SORT_COUNTS = NC_SORT_KEYS + 12 * 256
NC_SORT_SCRATCH = NC_SORT_COUNTS + 256
NC_END = NC_SORT_SCRATCH + 256
    ASSERT (FILE_INDEX_A & 0FFh) == 0 && (FILE_INDEX_B & 0FFh) == 0

;-----------------------------------------------------------------------
; Проверяем, что программа помещается в память вместе со своими буферами
;-----------------------------------------------------------------------

NC_END_MAX = APP_MEMORY_END
    ASSERT NC_END <= NC_END_MAX
    ASSERT NC_END_MAX <= bios_vars

    END
