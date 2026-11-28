;----------------------------------------------------------------------------
; MXOS NC.COM
; Рисование окошек
;
; 2013-12-18 Дизассемблировано vinxru
;----------------------------------------------------------------------------

drawWindowIfSel:
        call ncRealSelection
        pop hl
        jp c,mainLoop
        push hl
        ; Получить кол-во файлов
        ld      hl, state.panelA_filesCnt
        call    readBytePanel

        ; Если файлов нет, выходим
        and     a
        pop     hl ; Адрес возврата
        jp z,   mainDriveChanged
        push    hl
            

; ---------------------------------------------------------------------------

drawWindow1:
        ld hl,ncInputModal
        call ncModalOpen
        ld hl,240
        call ncProgressFieldWidth
        ld a,b
        ld (ncInputChars),a
        call ncModalInputCursor
        ld d,l
        ld hl,72
        ld bc,240
        ld a,(ncGlyphHeight)
        call uiInputFieldPixels
        call ncModalInputCursor
        call mx_setCursorPos
        ld a,1
        ld (mon2_vars.inverse),a
        ld hl,ncInputEmpty
        ld a,(ncInputChars)
        ld b,a
        call ncPrintField
        xor a
        ld (mon2_vars.inverse),a
        ret

; ---------------------------------------------------------------------------

; CF=нет обычного элемента. Псевдозаписи не являются целями операций.
; AF/HL разрушены; BC/DE сохраняются.
ncRealSelection:
        call getSelectedFile
        ld a,(hl)
        cp 0FFh
        jp z,ncNoSelection
        cp 05Ch
        jp z,ncNoSelection
        cp '.'
        jp z,ncNoSelection
        or a
        ret
ncNoSelection:
        scf
        ret


ncInputOkText:      DB 'OK [Enter]',0
ncInputCancelText:  DB 'Cancel [Esc]',0
ncInputModal:
    UI_MODAL_BEGIN 12,1,2
ncInputOk:
    UI_MODAL_BUTTON ncInputOkText,0,0,1
ncInputCancel:
    UI_MODAL_BUTTON ncInputCancelText,0,1,0
    UI_MODAL_END
ncInputEmpty: DB 0
