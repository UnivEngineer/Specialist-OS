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
        ld hl,g_window1
        call ncDialogOpen
        COLOR   COLOR_DIALOG

        ; Рисуем окно
        ld      hl, g_window1
        call    draw
      
        ; Рисуем прямоугольник
        ld      hl, 9F65h           ; x=120 y=101
        ld      bc, (18 << 8) + 11  ; w=144 h=11
        call    inverseRect
        ld hl,ncInputOk
        call ncDrawButton
        ld hl,ncInputCancel
        jp ncDrawButton

; ---------------------------------------------------------------------------

drawWindow2:
        ld hl,g_window2
        call ncDialogOpen
        COLOR   COLOR_DIALOG

        ; Рисуем окно
        ld      hl, g_window2
        call    draw

        ; Рисуем прямоугольник
        ld      hl, 9E87h           ; x=112 y=135
        ld      bc, (20 << 8) + 11  ; w=160 h=11
        jp      inverseRect

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

ncInputHint: DB "OK [Enter]  Cancel [Esc]",0

ncInputOkText: DB 'OK [Enter]',0
ncInputCancelText: DB 'Cancel [Esc]',0
ncInputOk: UI_BUTTON ncInputOkText,36,56,122,14,6,1
ncInputCancel: UI_BUTTON ncInputCancelText,100,56,122,14,6,0
