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
            ld      hl, 92F7h           ; адрес первого прямоугольника
            ld      d, 8                ; количество прямоугольников
colToolbarLoop:
            ld      bc, (4 << 8) + 9    ; b = ширина = 4 байта, c = высота 9 пикселей
            call    inverseRect
            inc     h
            inc     h
            dec     d
            jp nz,  colToolbarLoop

            ; Очищаем командную строку
            call    clearCmdLine

