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
            call  buildDriveList

            COLOR   COLOR_DIALOG

            ld    hl, g_chooseDrive
            call  draw

            ; Вывести заголовок и компактный список установленных драйверов.
            ld    hl, 1E63h
            call  setCursorPosPanel
            ld    hl, aChooseDrive
            call  bios_printString
            call  printDriveList

            ; Отключить курсор BIOS на время выбора.
            ld    a, (bios_vars.cursorCfg)
            push  af
            xor   a
            ld    (bios_vars.cursorCfg), a

            ; Найти текущий диск панели в компактном списке.
            ld    hl, state.panelA_drive
            call  readBytePanel
            ld    c, a
            call  findDrive
            jp nc, cd_StoreChoice
            xor   a
cd_StoreChoice:
            ld    (vars.chooseDrive), a

cd_Loop:
            ld    a, (vars.driveCount)
            or    a
            call  nz, chooseDrive_draw

            call  bios_getch

            push  af
            ld    a, (vars.driveCount)
            or    a
            call  nz, chooseDrive_draw
            pop   af

            ; В компактном списке вправо/вниз означает следующий диск,
            ; влево/вверх — предыдущий.
            ld    c, 1
            cp    18h
            jp z, cd_Arrows
            cp    1Ah
            jp z, cd_Arrows
            ld    c, 0FFh
            cp    8
            jp z, cd_Arrows
            cp    19h
            jp z, cd_Arrows
            cp    0Dh
            jp z, cd_Enter
            cp    1Bh
            jp z, cd_Esc

            ; Прямой выбор A-H разрешён только для найденного драйвера.
            sub   'A'
            cp    8
            jp nc, cd_Loop
            ld    c, a
            call  findDrive
            jp c, cd_Loop
            jp    cd_ReturnDrive

cd_Esc:
            pop   af
            ld    (bios_vars.cursorCfg), a
            ld    a, 0FFh
            ret

cd_Enter:
            ld    a, (vars.driveCount)
            or    a
            jp z, cd_Loop
            ld    a, (vars.chooseDrive)
            ld    e, a
            ld    d, 0
            ld    hl, vars.driveList
            add   hl, de
            ld    c, (hl)

cd_ReturnDrive:
            pop   af
            ld    (bios_vars.cursorCfg), a
            ld    a, c
            ret

cd_Arrows:
            ld    a, (vars.driveCount)
            or    a
            jp z, cd_Loop

            ld    a, (vars.chooseDrive)
            add   a, c
            cp    0FFh
            jp z, cd_ArrowLast
            ld    hl, vars.driveCount
            cp    (hl)
            jp c, cd_ArrowStore
            xor   a
cd_ArrowStore:
            ld    (vars.chooseDrive), a
            jp    cd_Loop

cd_ArrowLast:
            ld    a, (vars.driveCount)
            dec   a
            jp    cd_ArrowStore

;----------------------------------------------------------------------------
; Выбрать начальный диск правой панели. H имеет приоритет, если его драйвер
; уже установлен; иначе используется встроенный RAM-диск B.

initRightDrive:
            call  buildDriveList
            ld    c, 7
            call  findDrive
            ret nc
            ld    a, 1
            ld    (state.panelB_drive), a
            ret

;----------------------------------------------------------------------------
; Построить компактный список драйверов.
;
; Диск считается установленным, если функция 3 его драйвера возвращает
; ненулевое число секторов. Текущий диск DOS восстанавливается.
;
; Разрушает: AF, BC, DE, HL.

buildDriveList:
            ld    e, 2
            call  bios_fileGetSetDrive
            push  af

            xor   a
            ld    (vars.driveCount), a
            ld    (vars.chooseDrive), a

bdl_Scan:
            ld    a, (vars.chooseDrive)
            ld    e, 1
            call  bios_fileGetSetDrive
            ld    b, 3
            call  bios_diskDriver
            ld    a, d
            or    e
            jp z, bdl_Next

            ld    a, (vars.driveCount)
            ld    l, a
            ld    h, 0
            ld    de, vars.driveList
            add   hl, de
            ld    a, (vars.chooseDrive)
            ld    (hl), a
            ld    a, (vars.driveCount)
            inc   a
            ld    (vars.driveCount), a

bdl_Next:
            ld    a, (vars.chooseDrive)
            inc   a
            ld    (vars.chooseDrive), a
            cp    8
            jp c, bdl_Scan

            pop   af
            ld    e, 1
            call  bios_fileGetSetDrive
            ret

;----------------------------------------------------------------------------
; Найти диск в компактном списке.
;
; Вход:  C — номер диска 0-7.
; Выход: A — позиция в списке, CF=0; CF=1, если драйвера нет.
; Разрушает: B, E, HL.

findDrive:
            ld    a, (vars.driveCount)
            ld    b, a
            ld    hl, vars.driveList
            ld    e, 0

fd_Loop:
            ld    a, b
            or    a
            jp z, fd_NotFound
            ld    a, (hl)
            cp    c
            jp z, fd_Found
            inc   hl
            inc   e
            dec   b
            jp    fd_Loop

fd_Found:
            ld    a, e
            or    a
            ret

fd_NotFound:
            scf
            ret

;----------------------------------------------------------------------------
; Вывести до восьми найденных букв в сетке 4x2.
;
; Разрушает: AF, BC, DE, HL.

printDriveList:
            ld    hl, 1C74h
            call  setCursorPosPanel
            ld    hl, vars.driveList
            ld    a, (vars.driveCount)
            or    a
            ret z
            ld    b, a
            ld    d, 4

pdl_Loop:
            ld    a, (hl)
            inc   hl
            add   a, 'A'
            ld    c, a
            call  bios_printChar

            dec   b
            ret z
            dec   d
            jp z, pdl_NewRow

            ld    c, ' '
            call  bios_printChar
            call  bios_printChar
            call  bios_printChar
            jp    pdl_Loop

pdl_NewRow:
            ld    d, 4
            push  hl
            ld    hl, 1C7Fh
            call  setCursorPosPanel
            pop   hl
            jp    pdl_Loop

;----------------------------------------------------------------------------

chooseDrive_draw:
            ld    a, (vars.chooseDrive)
            ld    hl, 976Ch

            cp    4
            jp c, chooseDrive_draw2
            sub   4
            ld    c, a
            ld    a, l
            add   a, 0Bh
            ld    l, a
            ld    a, c
chooseDrive_draw2:
            ld    c, a
            add   a, a
            add   a, c
            add   a, h
            ld    h, a
            call  activePanelPos
            ld    bc, (1 << 8) + 9
            jp    inverseRect
