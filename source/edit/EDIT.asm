;----------------------------------------------------------------------------
; MXOS
; EDIT.COM 6.0 - текстовый редактор
;
; 2022-02-03 Дизассемблировано SpaceEngineer
;----------------------------------------------------------------------------

    INCLUDE "../include/mxos.inc"
    INCLUDE "../include/version.inc"
    IFNDEF EDIT_BANKED
EDIT_BANKED = 0
    ENDIF
    IFNDEF EDIT_HEX_PSEUDO
EDIT_HEX_PSEUDO = 0
    ENDIF
HEX_RENDER_PSEUDO = EDIT_HEX_PSEUDO
    IF EDIT_BANKED
IMAGE_FLAGS = IMAGE_FLAG_BANK
    INCLUDE "bank-api.inc"
    INCLUDE "bank-bindings.inc"
EDITOR_MEMORY_END = EDIT_BANK_QUEUE
    ELSE
IMAGE_FLAGS       = IMAGE_FLAG_MAIN
EDITOR_MEMORY_END = APP_MEMORY_END
    ENDIF

; Компиляция оригинального редаткора (если 1)
STANDARD_EDITOR = 0

; Используемые подпрограммы DOS.SYS:
; bios_beep_Old         = 0C170h
; bios_delay_b          = 0C190h
; bios_cmp_hl_de        = 0C427h
; bios_memcpy_bc_hl     = 0C42Dh
; bios_reboot           = 0C800h
; bios_getch            = 0C803h
; bios_printChar        = 0C809h
; bios_printString      = 0C818h
; bios_calcCS           = 0C82Ah
; bios_fileCreate       = 0C845h
; bios_fileLoadInfo     = 0C851h
; bios_fileNamePrepare  = 0C85Ah
; fileReadDescriptor        = 0C866h

; Используемые переменные DOS.SYS:
; bios_vars.lastKey     = 08FF0h
; bios_upper.cursorDelay = 08FF4h
; bios_vars.inverse     = 08FFAh
; bios_vars.cursorY     = 08FFCh

; Используемые порты устройств:
; IO_KEYB_A = 0FFE0h
; IO_KEYB_B = 0FFE1h

;----------------------------------------------------------------------------

; Цвета

COLOR_TEXT      = 0B1h
; Same palette as NC's function-key toolbar (preset 1).
COLOR_HELP_F     = 060h
COLOR_STATUSBAR  = 030h
EDITOR_TEXT_TOP  = 10
EDITOR_FIRST_Y   = EDITOR_TEXT_TOP+8
EDITOR_ROWS      = 23
EDITOR_HEIGHT    = EDITOR_ROWS*10
EDITOR_LAST_Y    = EDITOR_FIRST_Y+(EDITOR_ROWS-1)*10
EDITOR_PAGE_STEP = EDITOR_ROWS-2

; Когда цвет включен, макрос вставляет такой код
    MACRO COLOR x
        IF STANDARD_EDITOR==0
            ld a, x
            ld (IO_COLOR), a
        ENDIF
    ENDM

    MACRO HEX_ROW_TARGET
        IF EDIT_BANKED
        ld hl,(hexRowTarget)
        ELSE
        ld hl,hexRow
        ENDIF
    ENDM

;----------------------------------------------------------------------------

; Собственные переменные
    STRUCT EDITOR_VARIABLES
last    BLOCK  1,  0
    ENDS

; История имени и экземпляр переменных размещены после кода до текста.


;----------------------------------------------------------------------------

; Начало программы
    IF !STANDARD_EDITOR
        IF EDIT_BANKED
        ORG 0100h
        JP editorBankEntry
        ELSE
        ORG 0
        ENDIF
    ELSE
        ORG 0D000h
    ENDIF

;----------------------------------------------------------------------------

; Буферы в конце программы
; Начало текста выравнивается после кода; верхний предел берётся из memtop.

EDITOR_BUFFER_SIZE EQU EDITOR_MEMORY_END - EDITOR_TEXT_BEGIN

;----------------------------------------------------------------------------

    INCLUDE "../include/ui.inc"
    INCLUDE "startup.inc" ; Старт, данные сеанса и цикл строки состояния
editorShiftState: DB 0
    IF !EDIT_BANKED
    INCLUDE "keyboard.inc" ; Диспетчер клавиш и два фиксируемых набора
    INCLUDE "keyboard-handlers.inc"
    ENDIF
    INCLUDE "gap.inc" ; Логические адреса и перемещение зазора документа
    INCLUDE "columns.inc" ; Байтовые позиции, визуальные колонки и Tab
    INCLUDE "edit.inc" ; Редактирование строки и изменение размера текста
    INCLUDE "blocks.inc" ; Табуляция, страницы и выделение строк
    INCLUDE "search.inc" ; Поиск, разбиение и объединение строк, копирование блоков
    INCLUDE "document-scroll.inc"
    INCLUDE "screen.inc" ; Примитивы памяти, курсор, экран и строка состояния
    INCLUDE "files.inc" ; Дисковые операции, ввод имени и дескриптор файла
    INCLUDE "entry.inc" ; Файловые клавиши и вход с аргументом командной строки

    INCLUDE "hex-entry.inc"
    INCLUDE "selection.inc"
    INCLUDE "interface.inc"
    INCLUDE "hex-block.inc"
    DEFINE LINEEDIT_RECALL
    INCLUDE "../include/lineedit.inc"
    IF EDIT_BANKED
    INCLUDE "bank-runtime.inc"
    ELSE
    DEFINE UI_SCROLL_BAR
    DEFINE UI_PIXEL_RECT
    DEFINE UI_TEXT_RECT
    DEFINE UI_BYTE_RECT
    DEFINE UI_IMPLEMENTATION
    INCLUDE "../include/ui.inc"
    ENDIF
    INCLUDE "../include/ui-text-align.inc"
    INCLUDE "../include/ui-button.inc"
    IF EDIT_BANKED
    DEFINE UI_MODAL_SNAPSHOT ebSnapshotModal
    ELSE
    DEFINE UI_MODAL_SNAPSHOT hexSaveUnderlay
    INCLUDE "hex-underlay.inc"
    ENDIF
    DEFINE UI_MODAL_KEYS
    INCLUDE "../include/ui-modal.inc"
    IF EDIT_BANKED
    DEFINE UI_HELP_HANDLE ebGfxHandle
    DEFINE UI_HELP_REGS ebGfxRegisters
    DEFINE UI_HELP_BANK ebNumber
    DEFINE UI_HELP_PIXELS EDIT_MAIN_PIXELS
    DEFINE UI_HELP_KEY uiModalGetch
    INCLUDE "../include/ui-bank-help.inc"
    ELSE
    DEFINE UI_HELP_OPEN editorModalBegin
    DEFINE UI_HELP_KEY uiModalGetch
    DEFINE UI_HELP_CLOSE editorCloseModal
    DEFINE UI_HELP_COLOR editorHelpColor
    DEFINE UI_HELP_BUFFER inputBuf
    INCLUDE "../include/ui-help.inc"
    ASSERT MAX_PATH >= UI_HELP_BLOCK
    ENDIF
FILE_NAME_HISTORY: DS MAX_PATH,0
FILE_NAME_RECALL_END:
editor_vars EDITOR_VARIABLES

    IF !STANDARD_EDITOR
    DEFINE FILE_IO_DESCRIPTOR_ONLY
    INCLUDE "../include/file-io.inc"
    INCLUDE "../include/errors.inc"
    IF !EDIT_BANKED
    INCLUDE "hex-view.inc"
    EDIT_STARTUP_CREDIT
    ENDIF
    IF EDIT_BANKED
    INCLUDE "hex-cache.inc"
    INCLUDE "hex-packet.inc"
    INCLUDE "hex-eof.inc"
    EDIT_STARTUP_CREDIT
    ENDIF
EDITOR_TEXT_BEGIN = ($+255) & 0FF00h
        ASSERT EDITOR_TEXT_BEGIN < EDITOR_MEMORY_END-2
    IF EDIT_BANKED
; The builder splits the two linked overlays out of this assembly stream.
editorResidentEnd:
hexPacked:
        DISP EDIT_HEX_OVERLAY
    INCLUDE "hex-view.inc"
hexCacheTags: DS HEX_CACHE_ROWS,0
hexOverlayEnd:
        ENT
hexOverlayBytes = $-hexPacked
        ASSERT hexOverlayEnd <= EDIT_BANK_STATE
textPacked:
        DISP EDIT_TEXT_OVERLAY
    INCLUDE "text-overlay.inc"
textOverlayEnd:
        ENT
textOverlayBytes = $-textPacked
        ASSERT textOverlayEnd <= EDIT_BANK_STATE
IMAGE_MEM_END  = EDIT_BANK_STATE_END
IMAGE_BSS_SIZE = IMAGE_MEM_END-editorResidentEnd
    ELSE
IMAGE_MEM_END  = EDITOR_TEXT_BEGIN
IMAGE_BSS_SIZE = IMAGE_MEM_END-$
    ENDIF
IMAGE_MEMORY_POLICY = IMAGE_POLICY_MANAGED
IMAGE_API           = 2
    ENDIF
