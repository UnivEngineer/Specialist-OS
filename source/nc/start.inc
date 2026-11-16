;----------------------------------------------------------------------------
; MXOS NC.COM
; Запуск
;
; 2013-12-18 Дизассемблировано vinxru
;----------------------------------------------------------------------------

Start:
            xor a
            ld (ncDialogActive),a
            ld (ncListMask),a
            ld (ncStartupPrepared),a
            call bios_setFontEncoding ; Always restore KOI-8 before any NC output.
            ld hl,0
            ld (bios_vars.inverse),hl
            call ncStartupCopyright
ncStartupBannerReady:
            call ncLayoutInit
            call ncMemoryInit
            jp c,ncMemoryError
            xor a
            MXOS_CALL MXOS_DOS_SET_SHELL
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
            ld      (vars.restoreSelectedA), a
            ld      (vars.restoreSelectedB), a
            ld      (vars.savedSelectedA), a
            ld      (vars.savedSelectedB), a
            call    ncCatalogInit
            call    initRightDrive
            call    ncConfigLoad

            ; Apply the state overlay before reading either panel's catalog.
            ld hl,ncDiskChanged
            ld a,CLI_OP_OBSERVER
            MXOS_CALL MXOS_DOS_CLI
            call    loadState
            call ncPreparePanels
            ld a,1
            ld (ncStartupPrepared),a

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

