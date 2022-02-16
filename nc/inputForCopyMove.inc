;----------------------------------------------------------------------------
; MXOS NC.COM
; Ввод строки в которой изначально содержится имя выбранного файла
;
; 2013-12-18 Дизассемблировано vinxru
;----------------------------------------------------------------------------

inputForCopyMove:
            ; Положение курсора
            ld      hl, 3D6Eh
            ld      (bios_vars.cursorY), hl

            ; Строка для редактирования   
            ld      de, vars.input

            ; Отображается ли панель?
            ld      hl, state.panelA_info
            ld      a, (state.activePanel)
            and     a
            jp nz,      loc_D255
            inc     hl
loc_D255:   ld      a, (hl)

            ; Если не отображается, то накопитель в стркоу не помещать
            and     a
            jp nz,      loc_D265

            ; Записываем букву накопителя
            inc     hl
            inc     hl
            ld      a, (hl)
            add     'A'
            ld      (de),a
            inc     de

            ; Записываем ':'
            ld      a, ':'
            ld      (de),a
            inc     de

loc_D265:   ; Записать выбранный файл
            call    getSelectedFile
            call    compactName

            ; Записать терминатор
            dec     de  ; compactName добавляет в конец строки ' ', 0Dh
            dec     de
            xor     a
            ld      (de),a

            ; Перейти к редакированию
            jp      input

