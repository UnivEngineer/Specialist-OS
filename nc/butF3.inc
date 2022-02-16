;----------------------------------------------------------------------------
; MXOS NC.COM
; Клавиша F3 - отобразить информацию о накопителе
;
; 2013-12-18 Дизассемблировано vinxru
;----------------------------------------------------------------------------

but_F3:     ; Временный выбор противоположной панели
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
            and   a
            jp z, loc_D701

            ; Перерисовываем панель
            call  loadAndPrintA

            jp    saveStateAndReload

; ---------------------------------------------------------------------------

loc_D701:   call  loadAndPrintB
            jp    saveStateAndReload

