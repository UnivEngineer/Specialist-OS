;----------------------------------------------------------------------------
; MXOS NC.COM
; Клавиши F1, F2. Выбор накопителя для панели
;
; 2013-12-18 Дизассемблировано vinxru
;----------------------------------------------------------------------------

but_F2:
            ld a,(ncShiftState)
            or a
            jp z,ncTapeStub
            ld      c, 1
            call    choosePanelDrive

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

but_F1:
            ld a,(ncShiftState)
            or a
            jp z,ncTapeStub
            ld      c, 0
            call    choosePanelDrive

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

; Общий диалог F1/F2. C=целевая панель; прежняя активная панель восстановлена.
; AF/BC/DE/HL и флаги разрушаются. Отмена не изменяет параметры панели.
choosePanelDrive:
            ld      a, (state.activePanel)
            push    af
            ld      a, c
            ld      (state.activePanel), a
            call    chooseDrive
            cp      0FFh
            jp z,   cpd_Restore
            ld      hl, state.panelA_drive
            call    writeBytePanel
            call    resetPanelDirectory
            xor     a
            ld      hl, state.panelA_info
            call    writeBytePanel
            ld      hl, state.panelA_curFile
            call    writeBytePanel
            ld      hl, state.panelA_firstFile
            call    writeBytePanel
cpd_Restore:
            pop     af
            ld      (state.activePanel), a
            ret

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
            ld      hl, vars.driveList
            ld      bc, 8
            MXCALL MX_DOS_FILEGETDRIVES
            ld      a, b
            ld      (vars.driveCount), a
            xor     a
            ld      (vars.chooseDrive), a
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
