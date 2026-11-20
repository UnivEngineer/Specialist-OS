;----------------------------------------------------------------------------
; MXOS NC.COM
; Клавиша F5 - копирование
;
; 2013-12-18 Дизассемблировано vinxru
;----------------------------------------------------------------------------

but_F5: ; Рисуем окно
        ld a,CLI_OP_COPY
        ld (copyFileOperation+1),a
        call    drawWindowIfSel

        ; Выводм "COPYING FROM A: TO"
        ld hl,state.panelA_drive
        call readBytePanel
        add a,'A'
        ld (aCopyFromTo+10),a
        ld hl,aCopyFromTo
        call uiModalTitle

        call    inputForCopyMove
        cp      1Bh
        jp z,   ncRefreshDirty

        call    copyFileInt
        jp      ncRefreshDirty

