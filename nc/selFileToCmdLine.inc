;----------------------------------------------------------------------------
; MXOS NC.COM
; Сформировать строку пути выбраного файла
;
; вход:
;   hl - адрес дескриптора файла
;
; 2013-12-18 Дизассемблировано vinxru
;----------------------------------------------------------------------------

getSelFilePath:
        ex      de, hl

        ; Получаем текущий накопитель
        ld      hl, state.panelA_drive
        call    readBytePanel
        add     'A'

        ; Выводим его в ком строку
        ld      hl, (vars.cmdLineEnd)
        ld      (hl), a
        inc     hl

        ; Выводим двоеточние
        ld      (hl), ':'
        inc     hl

        ; Выводим имя файла
        ex      de, hl
        call    compactName

        ; Ограничение длины
        dec     de
        ld      a, vars.cmdLineCtrl & 0FFh
        cp      e
        jp nc,  loc_D146
        ld      e, a
loc_D146:   
        ; Терминатор
        xor     a
        ld      (de), a
        ret

;----------------------------------------------------------------------------
; MXOS NC.COM
; Поместить выбранный файл в ком строку
;
; вход:
;   hl - адрес дескриптора файла
;
; 2013-12-18 Дизассемблировано vinxru
;----------------------------------------------------------------------------

selFileToCmdLine:
        call    getSelFilePath

        ; Вывод
        COLOR   COLOR_CMDLINE
        ld      hl, (vars.cmdLineEnd)
        call    bios_printString

        ; Дописываем имя файла
        ex      de, hl
        ld      (vars.cmdLineEnd), hl

        ; Сохраняем новое положение
        ld      hl, (bios_vars.cursorY)
        ld      (vars.cmdLinePos), hl
        ret

