;+---------------------------------------------------------------------------
; MXOS
; Перезагрузка (продолжение)
;
; 2013-12-12 Дизассемблировано vinxru
;----------------------------------------------------------------------------

reboot1:    ; Для выключения звука
            ld    a, 36h

            ; Включаем ОЗУ
            ld    (IO_PAGE_RAM), a

            ; Выключаем звук        
            ld    (IO_TIMER_MODE), a

            ; Инициализация переменных
            ld    hl, initVars      ; Откуда
            ld    de, initVarsEnd   ; Конец
            ld    bc, bios_vars     ; Куда
            call  memcpy_bc_hl

            ; Установка цвета
    IF ENABLE_COLOR
            ld    a, COLOR_BIOS
            ld    (IO_COLOR), a
    ELSE
            ld    a, 0Fh  
            ld    (IO_KEYB_MODE), a 
    ENDIF

    IF LOAD_FONT
onceInitFont:
            jp    initFont
    ELSE
            jp    reboot2
    ENDIF

