;----------------------------------------------------------------------------
; MXOS NC.COM
; Клавиша F3 - отобразить информацию о накопителе
;
; 2013-12-18 Дизассемблировано vinxru
;----------------------------------------------------------------------------

but_F3:
            ld    a,(ncShiftState)
            or    a
            jp z, ncHelp

            ; Временный выбор противоположной панели
            ld    a, (state.activePanel)
            push  af
            xor   1
            ld    (state.activePanel),a

            ; Меняем видимость панели
            ld    hl, state.panelA_info
            call  readBytePanel
            xor   1
            call  writeBytePanel

            ; Восстанавлаием активную панель
            pop   af
            ld    (state.activePanel),a
            xor   1
            ld    c,a
            call  loadAndPrintPanel

            ; The active file selection and command prompt are already visible.
            ld    a,(ncDamageMask)
            or    a
            jp z, mainLoop_cursorReady
            ld    a,1
            ld    (ncCommandCursorKept),a
            jp    ncRefreshDirty

