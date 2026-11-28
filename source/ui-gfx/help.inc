; File-backed modal in MAIN. The caller keeps keyboard focus and feeds keys.
; A new graphics owner must not inherit an interrupted help session.
ngHelpReset:
    ld a,(ngHelpActive)
    or a
    ret z
    xor a
    ld (ngHelpActive),a
    jp flClose

ngHelpOpen:
    ld a,(ngHelpActive)
    or a
    jp nz,ngBad
    ld a,c
    or a
    jp z,ngBad
    cp MAX_PATH+1
    jp nc,ngBad
    push hl
    ex de,hl
    ld (ngHelpPixels),hl
    pop hl
    dec hl
    ld a,b
    ld (ngTransfer),a
    ld (ngTransfer+1),hl
    xor a
    ld (ngTransfer+3),a
    ld h,0
    ld l,c
    inc hl
    ld (ngTransfer+6),hl
    ld hl,ngHelpPalette
    ld (ngTransfer+4),hl
    ld hl,ngTransfer
    call mx_memcpyFar
    ret c
    ld hl,(ngTransfer+6)
    ld de,ngHelpPalette-1
    add hl,de
    ld a,(hl)
    or a
    jp nz,ngBad
    call ngHelpSaveCursor
    ld hl,ngHelpBuffer
    call uiShowHelp
    jp ngHelpResult
ngHelpKey:
    ld a,(ngHelpActive)
    or a
    jp z,ngBad
    call ngHelpSaveCursor
    ld a,b
    rrca
    ld a,c
    call uiHelpKey
ngHelpResult:
    push af
    jp c,ngHelpInactive
    xor 1
    jp ngHelpSetActive
ngHelpInactive:
    xor a
ngHelpSetActive:
    ld (ngHelpActive),a
    ld hl,(ngHelpCursor)
    ld (mon2_vars.cursorY),hl
    ld hl,(ngHelpInverse)
    ld (mon2_vars.inverse),hl
    pop af
    ret

;----------------------------------------------------------------------------
; MXOS
;
; Снять координаты курсора и атрибуты вывода перед работой со справкой.
;
; вход:
;   В MAIN доступны слова mon2_vars.cursorY и mon2_vars.inverse.
;
; выход:
;   Слова скопированы в ngHelpCursor и ngHelpInverse соответственно. af, bc и de
;   сохранены, hl содержит последнее считанное слово.
;----------------------------------------------------------------------------

ngHelpSaveCursor:
    ld hl,(mon2_vars.cursorY)
    ld (ngHelpCursor),hl
    ld hl,(mon2_vars.inverse)
    ld (ngHelpInverse),hl
    ret
ngHelpFrame:
    call ngHelpColor
    xor a
    ld (mon2_vars.inverse),a
    ld (mon2_vars.inverse+1),a
    ld a,(FONT_ENGINE_HEADER+5)
    jp uiModalOpen

;----------------------------------------------------------------------------
; MXOS
;
; Выбрать аппаратный цвет текста справки.
;
; вход:
;   Параметров нет.
;
; выход:
;   В IO_COLOR записано 07h, a = 07h. bc, de, hl и флаги сохранены.
;----------------------------------------------------------------------------

ngHelpColor:
    ld a,007h
    ld (IO_COLOR),a
    ret
ngHelpSnapshot:
    push hl
    ld hl,(ngHelpPixels)
    ld a,h
    or l
    pop hl
    ret z
    ; HL is the generated G_MODAL_WINDOW.
    inc hl
    ld e,(hl)
    inc hl
    ld d,(hl)
    inc hl
    ld a,(hl)
    add a,6
    ld c,a
    inc hl
    ld a,(hl)
    add a,2
    ld b,a
    ex de,hl
    ld (ngHelpScreen),hl
    push bc
    pop hl
    ld (ngHelpSize),hl
    ld hl,(ngHelpPixels)
    ex de,hl
    ld hl,(ngHelpScreen)
    jp uiBitmapSave
ngHelpClose:
    ld hl,(ngHelpPixels)
    ld a,h
    or l
    ret z
    ex de,hl
    ld hl,(ngHelpSize)
    ld b,h
    ld c,l
    ld hl,(ngHelpScreen)
    ld a,(ngHelpPalette)
    ld (IO_COLOR),a
    jp uiBitmapRestore
ngHelpActive:   DB 0
ngHelpPixels:   DW 0
ngHelpScreen:   DW 0
ngHelpSize:     DW 0
ngHelpCursor:   DW 0
ngHelpInverse:  DW 0
ngHelpPalette:  DB 0
ngHelpBuffer:   DS MAX_PATH,0

    INCLUDE "../../sdk/asm/lib/ui-text-align.inc"
    INCLUDE "../../sdk/asm/lib/ui-button.inc"
    DEFINE UI_MODAL_SNAPSHOT ngHelpSnapshot
    INCLUDE "../../sdk/asm/lib/ui-modal.inc"
    DEFINE FILE_IO_HANDLE_ONLY
    INCLUDE "../../sdk/asm/lib/file-io.inc"
    DEFINE UI_HELP_STEPPED
    DEFINE UI_HELP_OPEN ngHelpFrame
    DEFINE UI_HELP_CLOSE ngHelpClose
    DEFINE UI_HELP_COLOR ngHelpColor
    DEFINE UI_HELP_BUFFER ngHelpBuffer
    INCLUDE "../../sdk/asm/lib/ui-help.inc"
