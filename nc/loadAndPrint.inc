;----------------------------------------------------------------------------
; MXOS NC.COM
; Загрузить файлы и нарисовать
;
; 2013-12-18 Дизассемблировано vinxru
;----------------------------------------------------------------------------

loadAndPrintB:    ; Времено меняем активную панель на правую
            ld      a, (activePanel)
            push    af
            ld      a, 1
            ld      (activePanel),a

            ; Переходим к общему коду
            ld      a, (panelB_info)
            jp      loadAndPrint

; ---------------------------------------------------------------------------

loadAndPrintA:    ; Времено меняем активную панель на левую
            ld      a, (activePanel)
            push    af
            ld      a, 0
            ld      (activePanel),a

            ; Панель видима?
            ld      a, (panelA_info)
loadAndPrint:
            and     a
            jp nz,  loadAndPrint_2

            ; Загружаем список файлов
            ld      hl, panelA_drive
            call    readBytePanel
            call    loadFiles   ; На входе a должен содержать номер накопителя

            ; Выводим список файлов
            call    printFilePanel

            jp      loadAndPrint_3

; ---------------------------------------------------------------------------

loadAndPrint_2:   ; Рисуем панель информации
            call    printInfoPanel

loadAndPrint_3:   ; Восстанавливаем активную панель
            pop     af
            ld      (activePanel),a

            ; Загружаем список файлов
            ld      hl, panelA_drive
            call    readBytePanel
            call    loadFiles   ; На входе a должен содержать номер накопителя
            ret

