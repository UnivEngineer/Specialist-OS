;----------------------------------------------------------------------------
; MXOS NC.COM
; Клавиша TAB - Смена активной панели
;
; 2013-12-18 Дизассемблировано vinxru
;----------------------------------------------------------------------------

Tab:        ; Получаем видимость панели
            ld    hl, panelA_info
            ld    a, (activePanel)
            and   a
            jp nz,  loc_D6C7
            inc   hl
loc_D6C7:   ld    c, a
            ld  a, (hl)

            ; Панель не видима, выходим
            and   a
            jp nz,  mainDriveChanged

            ; Меняем активную панель на противоположную
            ld    a, c
            xor   1                       
            ld    (activePanel),a

            ; Перезагружаем списк файлов
            ld    hl, panelA_drive
            call  readBytePanel
            call  loadFiles   ; На входе нужен a=накопитель

            jp    loc_D603

