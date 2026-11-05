;----------------------------------------------------------------------------
; MXOS NC.COM
; Ввод строки в которой изначально содержится имя выбранного файла
;
; 2013-12-18 Дизассемблировано vinxru
;----------------------------------------------------------------------------

inputForRename:
            jp      inputForFile
inputForCopyMove:
            ; Полный путь каталога другой панели, а не зависимое от CWD A:имя.
            call    getSelectedFile
            push    hl
            call    inputSwitchPanel
            pop     hl
            call    buildSelectedFilePath
            push    af
            call    inputSwitchPanel
            call    updateCommandPath
            pop     af
            ld      a,01Bh
            ret c
            ld      hl,vars.pathArg
            ld      de,vars.input
inputCopyPath:
            ld      a,(hl)
            ld      (de),a
            inc     hl
            inc     de
            or      a
            jp nz,inputCopyPath
            dec     de
            jp      input

; Выбрать файловый контекст противоположной панели. AF/BC/DE/HL разрушены.
inputSwitchPanel:
            ld      a,(state.activePanel)
            xor     1
            ld      (state.activePanel),a
            ld      hl,state.panelA_drive
            call    readBytePanel
            call    bios_fileSelectDrive
            jp      restorePanelDirectory

inputForFile:
            ld      hl, 3D6Eh
            ld      (bios_vars.cursorY), hl
            ld      de, vars.input
            ; Записать выбранный файл
            call    getSelectedFile
            call    compactName

            ; Записать терминатор
            dec     de  ; compactName добавляет в конец строки ' ', 0Dh
            dec     de
            xor     a
            ld      (de),a

            ; Перейти к редакированию
            jp      input

