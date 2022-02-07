;----------------------------------------------------------------------------
; MXOS NC.COM
; Клавиши F1, F2. Выбор накопителя для панели
;
; 2013-12-18 Дизассемблировано vinxru
;----------------------------------------------------------------------------

F2:         ; Временно устанавливаем activePanel=1
            ld    a, (activePanel)
            push  af
            ld    a, 1
            ld    (activePanel),a

            ; Диалог
            call  chooseDrive

            ; Если выбрали устройство, то сохраняем его.
            cp    0FFh
            jp z, loc_D5C3          
            ld    (panelB_drive),a

            ; А так же делаем панель видимой и перемещаем курсор в начало
            xor   a
            ld    (panelB_info),a
            ld    (panelB_curFile),a

loc_D5C3:   ; Восстанавливаем activePanel
            pop   af
            ld    (activePanel),a

            ; Перезагружаем и перерисовываем вторую панель
            call  loadAndPrintB

            ; Перезагружаем и перерисовываем первую панель, только если она отображается
            ld    a, (panelA_info)
            and   a
            jp z, saveStateAndReload
            call  loadAndPrintA

            ; Сохраняем состояние и выходим
            jp    saveStateAndReload

; ---------------------------------------------------------------------------

F1:         ; Временно устанавливаем activePanel=02
            ld    a, (activePanel)
            push  af
            ld    a, 0
            ld    (activePanel),a

            ; Диалог
            call  chooseDrive

            ; Если выбрали устройство, то сохраняем его.
            cp    0FFh
            jp z, loc_D5F2
            ld    (panelA_drive),a

            ; А так же делаем панель видимой и перемещаем курсор в начало
            xor   a
            ld    (panelA_info),a
            ld    (panelA_curFile),a

loc_D5F2:   ; Восстанавливаем activePanel
            pop   af
            ld    (activePanel),a

            ; Перезагружаем и перерисовываем первую панель
            call  loadAndPrintA

            ; Перезагружаем и перерисовываем вторую панель, только если она отображается
            ld    a, (panelB_info)
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
            ld    hl, panelA_drive
            call  readBytePanel
            ld    (nc_vars.chooseDrive), a

loc_D644:   ; Рисуем курсор
            call  chooseDrive_draw

            ; Ждем нажатия клавиши
            call  bios_getch

            ; Стираем курсор
            push  af
            call  chooseDrive_draw
            pop   af

            ld    c, 1
            cp    18h
            jp z, loc_D690
            ld    c, 7
            cp    8
            jp z, loc_D690
            ld    c, 4
            cp    1Ah
            jp z, loc_D690
            cp    19h
            jp z, loc_D690
            cp    0Dh
            jp z, loc_D688
            cp    1Bh
            jp z, loc_D681

            ; Выбор диска нажатием на a-h
            sub   41h
            cp    8
            jp nc,      loc_D644
            ld    c, a

            ; Восстанавливаем вид курсора
            pop   af
            ld    (bios_vars.cursorCfg),a
            ld    a, c
            ret

; ---------------------------------------------------------------------------

loc_D681:   ; Восстанавливаем вид курсора
            pop   af
            ld    (bios_vars.cursorCfg),a

            ; Ничего не выбрано
            ld    a, 0FFh
            ret

; ---------------------------------------------------------------------------

loc_D688:   ; Восстанавливаем вид курсора
            pop   af
            ld    (bios_vars.cursorCfg),a
            ; Выбрали
            ld    a, (nc_vars.chooseDrive)
            ret

; ---------------------------------------------------------------------------

loc_D690:   ld    a, (nc_vars.chooseDrive)
            add   a, c
            and   7
            ld    (nc_vars.chooseDrive),a
            jp    loc_D644

; ---------------------------------------------------------------------------

chooseDrive_draw:
            ; Выбранное устройство
            ld    a, (nc_vars.chooseDrive)

            ; Координаты курсора
            ld    hl, 976Ch

            ; Вторая строка
            cp    4
            jp c, chooseDrive_draw2
            sub  4
            ld   c, a
            ld   a, l
            add  a,0Bh
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
            ld    b, 1        ; ширина 8 пикселей
            jp    inverseRect ; hl - адрес
            
