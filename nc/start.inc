;----------------------------------------------------------------------------
; MXOS NC.COM
; Запуск
;
; 2013-12-18 Дизассемблировано vinxru
;----------------------------------------------------------------------------

Start:      ; Отключаем инверсию
            ld      hl, 0
            ld      (bios_vars.inverse), hl

            ; Загружаем состояние. Если ZF, значит, уже запускали NC.COM.
            ; Иначе сохраняем состояние.
            call    loadState
            jp z,   formatCalled

            ; Сохраняем состояние
            call    saveState

; ---------------------------------------------------------------------------

formatCalled:
            ; Выводим строку с кнопками
            COLOR   COLOR_HELP_F
            ld      c, 1Fh  ; очистка экрана
            call    bios_printChar
            ld      hl, 1FFh
            ld      (bios_vars.cursorY), hl
            ld      hl, aF1LeftF2RighF3         
            call    bios_printString

            ; Раскрашиваем строку с кнопками
            COLOR   COLOR_HELP_TEXT
            ld      hl, 92F7h
    IF SHOW_F9
            ld      d, 9
    ELSE
            ld      d, 8
    ENDIF
loc_D049:
    IF SHOW_F9
            ld      b, 3
    ELSE
            ld      b, 4
    ENDIF
            call    inverseRect     ; hl - адрес, b - ширина, высота 9
            inc     h
            inc     h
            dec     d
            jp nz,  loc_D049

            ; Очищаем командную строку
            call    clearCmdLine

