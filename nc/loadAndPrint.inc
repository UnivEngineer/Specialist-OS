;----------------------------------------------------------------------------
; MXOS NC.COM
; Загрузить файлы и нарисовать
;
; 2013-12-18 Дизассемблировано vinxru
;----------------------------------------------------------------------------

loadAndPrintB:
            ; Времено меняем активную панель на правую
            ld      a, (state.activePanel)
            push    af
            ld      a, 1
            ld      (state.activePanel),a

            ; Переходим к общему коду
            ld      a, (state.panelB_info)
            jp      loadAndPrint

; ---------------------------------------------------------------------------

loadAndPrintA:
            ; Времено меняем активную панель на левую
            ld      a, (state.activePanel)
            push    af
            ld      a, 0
            ld      (state.activePanel),a

            ; Панель видима?
            ld      a, (state.panelA_info)

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
            call    loadFiles   ; На входе a должен содержать номер накопителя
            ret

