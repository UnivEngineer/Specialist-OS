;----------------------------------------------------------------------------

; MXOS - Файловый менеджер NC.COM

;

; 2013-12-18 Дизассемблировано и доработано vinxru

; 2022-01-31 Доработано SpaceEngineer

;----------------------------------------------------------------------------



    INCLUDE "../include/mxos.inc"

    IFNDEF NC_BANKED
NC_BANKED = 0
    ENDIF
    IF NC_BANKED
NC_CATALOG_SLOTS = 4
IMAGE_FLAGS      = IMAGE_FLAG_BANK
    INCLUDE "bank-api.inc"
    INCLUDE "bank-bindings.inc"
    ELSE
NC_CATALOG_SLOTS = 2
IMAGE_FLAGS      = IMAGE_FLAG_MAIN
    ENDIF

    INCLUDE "../include/ui.inc"



    IF NC_BANKED
    ORG 00100h
    jp ncBankEntry
    ELSE
    ORG NC_LOAD_ADDR
    ENDIF



; Список файлов на всю высоту панелей

    IFNDEF FULL_PANELS

FULL_PANELS = 0

    ENDIF



; 1: цветная битмап-прокрутка, 0: печать всех имён.

    IFNDEF NC_BITMAP_SCROLL

NC_BITMAP_SCROLL = 1

    ENDIF

    ASSERT NC_BITMAP_SCROLL == 0 || NC_BITMAP_SCROLL == 1

    IFNDEF NC_FILE_COLORS

NC_FILE_COLORS = ENABLE_COLOR

    ENDIF

    ASSERT NC_FILE_COLORS == 0 || NC_FILE_COLORS == 1

    ASSERT NC_FILE_COLORS == 0 || ENABLE_COLOR



    INCLUDE "style.inc"

    INCLUDE "start.inc"     ; Продолжается в main

    INCLUDE "main.inc"

    INCLUDE "selFileToCmdLine.inc"

    INCLUDE "butF4.inc"

    INCLUDE "directories.inc"

    INCLUDE "dirty.inc"

    INCLUDE "butEnter.inc"

    INCLUDE "saveLoadState.inc"

    INCLUDE "config.inc"

    INCLUDE "drawWindow.inc"

    INCLUDE "dialogBitmap.inc"

    INCLUDE "printStringInv.inc"

    INCLUDE "inputForCopyMove.inc"

    INCLUDE "printSelDrive.inc"

    INCLUDE "butF7.inc"

    INCLUDE "butF6.inc"

    INCLUDE "butF5.inc"

    INCLUDE "copyFileInt.inc"

    INCLUDE "fileops.inc"

    INCLUDE "overwrite.inc"


    INCLUDE "../include/errors.inc"




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


    IF NC_BANKED
    INCLUDE "bank-graphics.inc"
    ELSE
    DEFINE UI_PIXEL_RECT

    DEFINE UI_BYTE_RECT

    DEFINE UI_BITMAP

    DEFINE UI_TEXT_RECT
    IF NC_BITMAP_SCROLL



    IF NC_FILE_COLORS

    DEFINE UI_TEXT_RECT_COLORS

    ENDIF

    ENDIF

    DEFINE UI_MODAL_BORDER

    DEFINE UI_IMPLEMENTATION

    INCLUDE "../include/ui.inc"


    UNDEFINE UI_PIXEL_RECT

    UNDEFINE UI_BYTE_RECT

    UNDEFINE UI_BITMAP

    IF NC_BITMAP_SCROLL

    UNDEFINE UI_TEXT_RECT

    IF NC_FILE_COLORS

    UNDEFINE UI_TEXT_RECT_COLORS

    ENDIF

    ENDIF

    ENDIF

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

    IF NC_FILE_COLORS

    INCLUDE "fileColors.inc"

    ENDIF



    MODULE ncMath

    DEFINE MATH_DIV2

    DEFINE MATH_DIV

    DEFINE MATH_MUL

    INCLUDE "../include/math.inc"

    ENDMODULE



    IF NC_BANKED
    INCLUDE "bank-runtime.inc"
    ELSE
    INCLUDE "memory.inc"
    ENDIF

    INCLUDE "../include/ui-text-align.inc"
    INCLUDE "align.inc"
    IF ENABLE_COLOR
    DEFINE UI_BUTTON_COLOR ncSetColor
    ENDIF
    INCLUDE "../include/ui-button.inc"
    DEFINE UI_MODAL_SNAPSHOT ncModalSnapshot
    DEFINE UI_MODAL_BUTTON_DRAW ncDrawButton
    DEFINE UI_MODAL_KEYS
    INCLUDE "../include/ui-modal.inc"
    INCLUDE "modal.inc"
    INCLUDE "../include/file-io.inc"
    IF !NC_BANKED
    INCLUDE "../include/ui-scrollbar.inc"
    ENDIF
    IF NC_BANKED
    DEFINE UI_HELP_HANDLE ncGfxHandle
    DEFINE UI_HELP_REGS ncGfxRegisters
    DEFINE UI_HELP_BANK ncBankNumber
    DEFINE UI_HELP_PIXELS 0
    DEFINE UI_HELP_KEY uiModalGetch
    INCLUDE "../include/ui-bank-help.inc"
    ELSE
    DEFINE UI_HELP_OPEN ncModalOpen
    DEFINE UI_HELP_KEY uiModalGetch
    DEFINE UI_HELP_CLOSE ncDialogClose
    DEFINE UI_HELP_COLOR ncHelpColor
    DEFINE UI_HELP_BUFFER vars.pathArg
    INCLUDE "../include/ui-help.inc"
    ASSERT MAX_PATH >= UI_HELP_BLOCK
    ENDIF
    INCLUDE "help.inc"
    INCLUDE "layout.inc"
    INCLUDE "toolbar-text.inc"

    IF NC_BANKED
    INCLUDE "bank-toolbar.inc"
    ELSE
    INCLUDE "toolbar.inc"
    ENDIF



;---------------------------------------------------------------------------

; Константы и переменные

;---------------------------------------------------------------------------



    IF NC_BANKED
aCommanderVer:      DB "Commander 3.0 BANK",0
    ELSE
aCommanderVer:      DB "Commander 3.0",0
    ENDIF

aCopyright:         DB "(C) Omsk/SPb 1992-2026",0

aFileProtected:     DB "System or read-only file",0

aChooseDrive:       DB "Choose drive",0

aDeleteFrom:        DB "Delete from A:",0

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



aCopyFromTo:        DB "Copy from A: to",0

aCopyFailed:        DB "Copy failed",0

aFileAlreadyExists: DB "File already exists",0

aDeleteFailed:      DB "Delete failed",0

aCantRenameFile:    DB "Can",39,"t rename file!",0

aRenameOn:          DB "Rename on ",0

aMainRAM:           DB "bytes MAIN RAM",0

aMainFree:          DB "bytes MAIN max",0

aBankRAM:           DB "KiB BANK RAM",0

aBankPages:         DB "pages BANK RAM",0

aReservedPages:     DB "pages reserved",0

aFreePages:         DB "pages free",0

aDiskTotal:         DB "KiB total on ",0

aDiskFree:          DB "KiB free in ",0

aVolumeLabel:       DB "Label",0

aDiskDriver:        DB "Driver",0

aDiskTechnology:    DB "Type",0

aTypeROM:           DB "ROM",0

aTypeRAM:           DB "RAM",0

aTypeFlash:         DB "Flash",0
aTypeSystemFlash:       DB "MX System Flash",0
aTypeSystemRom:         DB "MX System ROM",0
aTypeVv55Flash:         DB "MX VV55 Flash",0
aTypeVv55Rom:           DB "MX VV55 ROM",0

aTypeFloppy:        DB "Floppy",0

aDriverUnknown:     DB "n/a",0,0

aDrive:             DB "Drive ",0

aNotFormatted:      DB " ","is not formatted",0
aNoMedia:           DB "No media",0





aEditor:            DB BOOT_VOLUME_LETTER,':',05Ch,"DOS",05Ch,"EDIT_MN.COM -NC",0Dh      ; терминатором тут должно быть 0Dh



;-----------------------------------------------------------------------

; Описания окон





g_filePanel:

            G_WINDOW 0, 0, 4, 192, 226        ; 2, 0, 90h, 0E0h, 16h

            DB 0

g_fileDividers:

            G_LINE   0, 4, 208, 184           ; 1, 0D0h, 90h, 16h, 0Fh, 0F0h

            G_VLINE  0, 96, 7, 201            ; 3, ?, 9Ch, 0CDh, 80h

            DB 0



g_infoPanel:

            G_WINDOW 0, 0, 4, 192, 226        ; 2, 0, 90h, 0E0h, 16h

g_infoDividers:

            G_LINE   0, 4, 31, 184            ; 1, 1Fh, 90h, 16h, 0Fh, 0F0h

            G_LINE   0, 4, 104, 184

            G_LINE   0, 4, 140, 184

            G_LINE   0, 4, 204, 184

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

; Internal panel fields; persistence uses the common textual configuration.



    STRUCT NC_STATE

initState           DB    5Bh   ; признак, что NC.COM уже запускался

activePanel         DB    0     ; номер активной панели

panelA_dirLow       DB    0

panelB_dirLow       DB    0

panelA_drive        DB    BOOT_VOLUME     ; номер диска панели A

panelB_drive        DB    7     ; номер диска панели B

panelA_dirHigh      DB    0

panelB_dirHigh      DB    0

panelA_firstFile    DB    0     ; c какого файла начинается панель A

panelB_firstFile    DB    0     ; c какого файла начинается панель B

panelA_curFile      DB    0     ; на каком файле курсор на панели A

panelB_curFile      DB    0     ; на каком файле курсор на панели B

panelA_sort         DB    2     ; 0=порядок диска, 1=имя/расширение, 2=расширение/имя

panelB_sort         DB    2

panelA_info         DB    0     ; 0=files, 1=F3 information panel.

panelB_info         DB    0

    ENDS



state   NC_STATE

stateEnd:           ; адрес конца блока переменных состояния



; Другие переменные - не включены в бинарник (машинный код не

; генерируется, при первом запуске переменные могут содержать мусор)



    STRUCT NC_VARIABLES

panelA_filesCnt     DB                 0

panelB_filesCnt     DB                 0
panelA_diskError    DB                 0
panelB_diskError    DB                 0

path                BLOCK              MAX_PATH,0

pathA               BLOCK              MAX_PATH,0           ; Полный путь левой панели.

pathB               BLOCK              MAX_PATH,0           ; Полный путь правой панели.

pathValidA          DB                 0

pathValidB          DB                 0

pathDriveA          DB                 0

pathDriveB          DB                 0

pathLowA            DB                 0

pathLowB            DB                 0

pathHighA           DB                 0

pathHighB           DB                 0

drawnLeftA          DB                 0                    ; Занятые строки в первой колонке.

drawnLeftB          DB                 0

drawnRightA         DB                 0                    ; Занятые строки во второй колонке.

drawnRightB         DB                 0

newRows             DB                 0                    ; Временный размер видимого списка.

rowColors           BLOCK              6*P_FILE_LIST_H,0    ; Displayed cells, including spare columns.

pathArg             BLOCK              MAX_PATH,0

displayPath         BLOCK              MAX_PATH,0

fileListPtr         DW                 0                    ; Видимое окно индексной таблицы активной панели.

cacheValidA         DB                 0

cacheValidB         DB                 0

cacheSlotA          DB                 0FFh                 ; Номер общего каталога; FF — ещё не назначен.

cacheSlotB          DB                 0FFh

panelFrameA         DB                 0                    ; 1 while the painted file panel is truncated.

panelFrameB         DB                 0

cacheSortA          DB                 0

cacheSortB          DB                 0

fileColorRules      BLOCK              4*32+1,0             ; Runtime NC.CFG extension rules, terminated by 0.

savedSelectedA      BLOCK              13,0

savedSelectedB      BLOCK              13,0

restoreSelectedA    DB                 0

restoreSelectedB    DB                 0

cmdLinePos          DW                 0

cmdLineEnd          DW                 0

cmdTextPos          DW                 0                    ; Начало видимой части команды после приглашения.

cmdCapacity         DB                 0                    ; Число видимых символов команды.

cmdOffset           DB                 0                    ; Смещение видимого окна в буфере команды.

chooseDrive         DB                 0

copyRequest         FILE_COPY_REQUEST

fileJob             FILEOPS_JOB

scrollCursorKept    DB                 0                    ; Старый курсор остаётся на краю при шаге 1.

scrollRows          DB                 0                    ; Модуль изменения начала списка (1..255).

scrollPixels        DB                 0                    ; Высота новых строк, не более одной колонки.

scrollRemain        DB                 0                    ; Высота сохраняемой части колонки.

diskInfoPtr         DW                 0                    ; адрес структуны DISK_INFO

driveCount          DB                 0                    ; число установленных дисковых драйверов

driveList           BLOCK              8, 0                 ; номера дисков с установленным драйвером



input               BLOCK              MAX_PATH, 0

    ENDS



vars    NC_VARIABLES = $        ; адрес начала блока переменнных

varsEnd = vars + NC_VARIABLES

state.panelA_filesCnt = vars.panelA_filesCnt

state.panelB_filesCnt = vars.panelB_filesCnt

vars.tempFileDescr = vars.copyRequest.destination



; Все рабочие буферы принадлежат NC и находятся после его кода.

FILE_CACHE_CAPACITY   = 255     ; Нынешние индексы NC — 8-разрядные.

FILE_CACHE_BYTES      = (FILE_CACHE_CAPACITY + 1) * FILE_INFO_SIZE

    ASSERT FILE_INFO_SIZE == 16

; Keep fixed, page-aligned sorting/index workspaces below the resizable tail.

vars.cmdLine = CMD_LINE

vars.cmdLineCtrl = CMD_LINE + 127

NC_TOOLBAR_CACHE = varsEnd

NC_TOOLBAR_CACHE_BYTES = 48*9

    IF NC_BANKED
FILE_INDEX_A = (varsEnd + 255) & 0FF00h ; Toolbar bitmaps use client-owned MAIN memory.
    ELSE
FILE_INDEX_A = (NC_TOOLBAR_CACHE + 2*NC_TOOLBAR_CACHE_BYTES + 255) & 0FF00h
    ENDIF

FILE_INDEX_B = FILE_INDEX_A + 256

CATALOG_INDEX_A = FILE_INDEX_B + 256

NC_SORT_KEYS = CATALOG_INDEX_A + NC_CATALOG_SLOTS * 256
    IF NC_BANKED
ncQSnapshot = NC_SORT_KEYS ; Palette runs reuse idle radix scratch; no DLL BSS.
    ENDIF

NC_SORT_RANKS = NC_SORT_KEYS + 11 * 256

NC_SORT_COUNTS = NC_SORT_KEYS + 12 * 256

NC_SORT_SCRATCH = NC_SORT_COUNTS + 256

FILE_CACHE_A = NC_SORT_SCRATCH + 768

NC_DIALOG_PIXELS = NC_SORT_KEYS

NC_DIALOG_COLORS = NC_SORT_KEYS+3072

NC_DIALOG_COLOR_BYTES = FILE_CACHE_A-NC_DIALOG_COLORS

    ASSERT NC_DIALOG_COLOR_BYTES >= 1024

FILE_CACHE_B = FILE_CACHE_A + FILE_CACHE_BYTES

FILE_CACHE_C = FILE_CACHE_B + FILE_CACHE_BYTES

NC_END = FILE_CACHE_A + NC_CATALOG_SLOTS * FILE_CACHE_BYTES

NC_MIN_CATALOG_CAPACITY = P_FILE_LIST_H ; One full column per catalog at minimum.

NC_MIN_END = FILE_CACHE_A + NC_CATALOG_SLOTS * (NC_MIN_CATALOG_CAPACITY + 1) * FILE_INFO_SIZE

    IF !NC_BANKED
; Fixed state, toolbar, indices and shared sort/dialog scratch are ordinary
; image BSS. Only the two variable-capacity catalogs use the MAIN heap.
IMAGE_BSS_SIZE      = FILE_CACHE_A-vars
IMAGE_MIN_HEAP_SIZE = NC_MIN_END-FILE_CACHE_A
    ENDIF

    ASSERT (FILE_INDEX_A & 0FFh) == 0 && (FILE_INDEX_B & 0FFh) == 0



;-----------------------------------------------------------------------

; Проверяем, что программа помещается в память вместе со своими буферами

;-----------------------------------------------------------------------



    IF NC_BANKED
NC_END_MAX = 0D000h
    ELSE
NC_END_MAX = APP_MEMORY_END
    ENDIF

    ; Catalog capacity is selected at startup for the actual memTop.

    ASSERT NC_MIN_END <= NC_END_MAX

    IF NC_BANKED
    ASSERT NC_END < ncBankQueue
    ELSE
    ASSERT NC_END_MAX <= bios_vars
    ENDIF



    END
