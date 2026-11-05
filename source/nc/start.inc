;----------------------------------------------------------------------------
; MXOS NC.COM
; Запуск
;
; 2013-12-18 Дизассемблировано vinxru
;----------------------------------------------------------------------------

Start:
            xor a
            call bios_setShell
            ; Модуль загружается до первого графического вызова.
            call    bios_loadDosModule
            jp c,   moduleLoadError
            ; Отключаем инверсию
            ld      hl, 0
            ld      (bios_vars.inverse), hl
            xor     a
            ld      (vars.path), a
            ld      (vars.scrollCursorKept), a
            ld      (vars.cacheValidA), a
            ld      (vars.cacheValidB), a
            ld      (vars.panelA_info), a
            ld      (vars.panelB_info), a
            ld      (bios_vars.koi7), a    ; Имена файлов и содержимое — KOI-8.

            ; Загружаем состояние. Если ZF, значит, уже запускали NC.COM.
            ; Иначе сохраняем состояние.
            ld hl,ncDiskChanged
            ld a,CLI_OP_OBSERVER
            call bios_cli
            call    loadState
            jp z,   formatCalled

            ; loadState оставляет побайтовый драйвер ДОЗУ при первом запуске.
            ld      b, 0
            call    bios_setRAMDDriver

            ; H используется по умолчанию только при установленном драйвере.
            call    initRightDrive

            ; Сохраняем исправленное начальное состояние.
            call    saveState

; ---------------------------------------------------------------------------

formatCalled:
            ; Выводим строку с кнопками
            COLOR   COLOR_HELP_F
            ld      c, 1Fh  ; очистка экрана
            call    bios_printChar
            call    drawToolbar

            ; Очищаем командную строку
            call    clearCmdLine

