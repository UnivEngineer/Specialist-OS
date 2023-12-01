;----------------------------------------------------------------------------
; MXOS NC.COM
; Клавиши F1, F2. Выбор накопителя для панели
;
; 2013-12-18 Дизассемблировано vinxru
;----------------------------------------------------------------------------

but_F2:     ; Временно устанавливаем state.activePanel=1
            ld    a, (state.activePanel)
            push  af
            ld    a, 1
            ld    (state.activePanel), a

            ; Диалог
            call  chooseDrive

            ; Если выбрали устройство, то сохраняем его
            cp    0FFh
            jp z, loc_D5C3          
            ld    (state.panelB_drive), a

            ; А также делаем панель видимой и перемещаем курсор в начало
            xor   a
            ld    (state.panelB_info), a
            ld    (state.panelB_curFile), a
            ld    (state.panelB_firstFile), a

loc_D5C3:   ; Восстанавливаем state.activePanel
            pop   af
            ld    (state.activePanel), a

            ; Перезагружаем и перерисовываем вторую панель
            call  loadAndPrintB

            ; Перезагружаем и перерисовываем первую панель, только если она в режиме информации
            ld    a, (state.panelA_info)
            and   a
            jp z, saveStateAndReload
            call  loadAndPrintA

            ; Сохраняем состояние и выходим
            jp    saveStateAndReload

; ---------------------------------------------------------------------------

but_F1:     ; Временно устанавливаем state.activePanel=2
            ld    a, (state.activePanel)
            push  af
            ld    a, 0
            ld    (state.activePanel), a

            ; Диалог
            call  chooseDrive

            ; Если выбрали устройство, то сохраняем его
            cp    0FFh
            jp z, loc_D5F2
            ld    (state.panelA_drive), a

            ; А также делаем панель видимой и перемещаем курсор в начало
            xor   a
            ld    (state.panelA_info), a
            ld    (state.panelA_curFile), a
            ld    (state.panelA_firstFile), a

loc_D5F2:   ; Восстанавливаем state.activePanel
            pop   af
            ld    (state.activePanel), a

            ; Перезагружаем и перерисовываем первую панель
            call  loadAndPrintA

            ; Перезагружаем и перерисовываем вторую панель, только если она в режиме информации
            ld    a, (state.panelB_info)
            and   a
            jp z, saveStateAndReload
            call  loadAndPrintB
            
saveStateAndReload:
            ; Сохраняем состояние и выходим
            call  saveState
            jp    mainDriveChanged

; ---------------------------------------------------------------------------

chooseDrive:
            COLOR   COLOR_DIALOG

            ld    hl, g_chooseDrive
            call  draw

            ; Выводим текст "CHOOSE DRIVE:"
            ld    hl, 1E63h
            call  setCursorPosPanel
            ld    hl, aChooseDrive
            call  bios_printString

            ; Выводим текст "A   B   C   D"
            ld    hl, 1C74h
            call  setCursorPosPanel
            ld    hl, aABCD
            call  bios_printString

            ; Выводим текст "E   F   G   H"
            ld    hl, 1C7Fh
            call  setCursorPosPanel
            ld    hl, aEFGH   
            call  bios_printString

            ; Меняем вид курсора
            ld    a, (bios_vars.cursorCfg)
            push  af
            xor   a
            ld    (bios_vars.cursorCfg), a

            ; Сохраняем текущее устройство
            ld    hl, state.panelA_drive
            call  readBytePanel
            ld    (vars.chooseDrive), a

cd_Loop:    ; Рисуем курсор
            call  chooseDrive_draw

            ; Ждем нажатия клавиши
            call  bios_getch

            ; Стираем курсор
            push  af
            call  chooseDrive_draw
            pop   af

            ; Выбор диска клавишами крсора
            ld    c, 1
            cp    18h       ; вправо
            jp z, cd_Arrows
            ld    c, 7
            cp    8         ; влево
            jp z, cd_Arrows
            ld    c, 4
            cp    1Ah       ; вниз
            jp z, cd_Arrows
            cp    19h       ; вверх
            jp z, cd_Arrows
            cp    0Dh       ; Enter
            jp z, cd_Enter
            cp    1Bh       ; Esc
            jp z, cd_Esc

            ; Выбор диска клавишами a-h
            sub   41h
            cp    8
            jp nc, cd_Loop
            ld    c, a

            ; Восстанавливаем вид курсора
            pop   af
            ld    (bios_vars.cursorCfg),a
            ld    a, c
            ret

; ---------------------------------------------------------------------------

cd_Esc:     ; Восстанавливаем вид курсора
            pop   af
            ld    (bios_vars.cursorCfg),a

            ; Ничего не выбрано
            ld    a, 0FFh
            ret

; ---------------------------------------------------------------------------

cd_Enter:   ; Восстанавливаем вид курсора
            pop   af
            ld    (bios_vars.cursorCfg),a
            ; Выбрали
            ld    a, (vars.chooseDrive)
            ret

; ---------------------------------------------------------------------------

cd_Arrows:  ld    a, (vars.chooseDrive)
            add   a, c
            and   7
            ld    (vars.chooseDrive),a
            jp    cd_Loop

; ---------------------------------------------------------------------------

chooseDrive_draw:
            ; Выбранное устройство
            ld    a, (vars.chooseDrive)

            ; Координаты курсора
            ld    hl, 976Ch

            ; Вторая строка
            cp    4
            jp c, chooseDrive_draw2
            sub  4
            ld   c, a
            ld   a, l
            add  a, 0Bh
            ld   l, a
            ld   a, c
chooseDrive_draw2:      
            ; Рисуем
            ld    c, a
            add   a, a
            add   a, c
            add   a, h
            ld    h, a
            call  activePanelPos
            ld    bc, (1 << 8) + 9  ; ширина 1 байт, высота 9 пикселей
            jp    inverseRect       ; hl = адрес
            
