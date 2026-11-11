;----------------------------------------------------------------------------
; MXOS NC.COM
; Запуск
;
; 2013-12-18 Дизассемблировано vinxru
;----------------------------------------------------------------------------

Start:
            xor a
            MXOS_CALL MXOS_DOS_SET_SHELL
            ; Отключаем инверсию
            ld      hl, 0
            ld      (bios_vars.inverse), hl
            xor     a
            ld      (vars.path), a
            ld      (vars.scrollCursorKept), a
            ld      (vars.cacheValidA), a
            ld      (vars.cacheValidB), a
            ld      (vars.pathValidA), a
            ld      (vars.pathValidB), a
            ld      (vars.drawnLeftA), a
            ld      (vars.drawnLeftB), a
            ld      (vars.drawnRightA), a
            ld      (vars.drawnRightB), a
            ld      (vars.panelA_info), a
            ld      (vars.panelB_info), a
            call bios_setFontEncoding    ; Имена файлов и содержимое — KOI-8.
            call    ncCatalogInit
            call    ncFontRenderEnd

            ; Загружаем состояние. Если ZF, значит, уже запускали NC.COM.
            ; Иначе используем начальное состояние в памяти.
            ld hl,ncDiskChanged
            ld a,CLI_OP_OBSERVER
            MXOS_CALL MXOS_DOS_CLI
            call    loadState
            jp z,   formatCalled

            ; H используется по умолчанию только при установленном драйвере.
            call    initRightDrive

            ; До запуска программы или выхода запись на B: не нужна.

; ---------------------------------------------------------------------------

formatCalled:
            ; Выводим строку с кнопками
            COLOR   COLOR_HELP_F
            ld      c, 1Fh  ; очистка экрана
            call    bios_printChar
            call    drawToolbar

            ; Очищаем командную строку
            call    clearCmdLine

