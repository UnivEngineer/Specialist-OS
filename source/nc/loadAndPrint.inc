;----------------------------------------------------------------------------
; MXOS NC.COM
; Загрузить файлы и нарисовать
;
; 2013-12-18 Дизассемблировано vinxru
;----------------------------------------------------------------------------

loadAndPrintB:
            ld      c, 1
            jp      loadAndPrintPanel
loadAndPrintA:
            ld      c, 0
loadAndPrintPanel:
            ld      a, (state.activePanel)
            push    af
            ld      a, c
            ld      (state.activePanel), a
            ld      hl, state.panelA_info
            call    readBytePanel

loadAndPrint:
            and     a
            jp nz,  loadAndPrint_2

            ; Выводим список файлов
            call    redrawFilePanel

            jp      loadAndPrint_3

; ---------------------------------------------------------------------------

loadAndPrint_2:   ; Рисуем панель информации
            call    printInfoPanel

loadAndPrint_3:   ; Восстанавливаем активную панель
            pop     af
            ld      (state.activePanel),a

            ; Загружаем список файлов
            ld      hl, state.panelA_drive
            call    readBytePanel
            jp      loadFiles
