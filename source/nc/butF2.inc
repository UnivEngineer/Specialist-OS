;----------------------------------------------------------------------------
; MXOS NC.COM
; Клавиши F1, F2. Выбор накопителя для панели
;
; 2013-12-18 Дизассемблировано vinxru
;----------------------------------------------------------------------------

but_F2:
            ld a,(ncShiftState)
            or a
            ld      c, 1
            jp z,ncPanelSort
            call    choosePanelDrive
            jp c,ncDriveDialogCancelled

            ; Перезагружаем и перерисовываем вторую панель
            call  loadAndPrintB

            ; Перезагружаем и перерисовываем первую панель, только если она в режиме информации
            ld    a, (state.panelA_info)
            and   a
            jp z, panelStateChanged
            call  loadAndPrintA

            ; Продолжаем работу с состоянием в памяти NC
            jp    panelStateChanged

; ---------------------------------------------------------------------------

but_F1:
            ld a,(ncShiftState)
            or a
            ld      c, 0
            jp z,ncPanelSort
            call    choosePanelDrive
            jp c,ncDriveDialogCancelled

            ; Перезагружаем и перерисовываем первую панель
            call  loadAndPrintA

            ; Перезагружаем и перерисовываем вторую панель, только если она в режиме информации
            ld    a, (state.panelB_info)
            and   a
            jp z, panelStateChanged
            call  loadAndPrintB
            
panelStateChanged:
            ; Продолжаем работу с состоянием в памяти NC
            ld a,(ncDamageMask)
            or a
            jp nz,ncRefreshDirty
            jp    mainDriveChanged
ncDriveDialogCancelled:
            ld a,(ncDamageMask)
            or a
            jp nz,ncRefreshDirty
            jp mainLoop

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
            jp z,   cpd_Cancel
            ld      b,a
            ld      hl,state.panelA_drive
            call    readBytePanel
            cp      b
            jp nz,  cpd_Change
            ld      hl,state.panelA_dirLow
            call    readBytePanel
            ld      d,a
            ld      hl,state.panelA_dirHigh
            call    readBytePanel
            or      d
            jp nz,  cpd_Change
            ld      hl,state.panelA_info
            call    readBytePanel
            or      a
            jp z,   cpd_Cancel       ; Root of this drive is already displayed.
cpd_Change:
            ld      a,b
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
            or a
            ret
cpd_Cancel:
            pop af
            ld (state.activePanel),a
            scf
            ret

chooseDrive:
            call  buildDriveList
            ld    a,(vars.driveCount)
            or    a
            jp z, cd_Esc
            ; Найти текущий диск панели в компактном списке.
            ld    hl, state.panelA_drive
            call  readBytePanel
            ld    c, a
            call  findDrive
            jp nc, cd_StoreChoice
            xor   a
cd_StoreChoice:
            ld    (vars.chooseDrive),a
            call ncDriveModalOpen
cd_Loop:
            call uiModalReadChoice
            jp nc,cd_Enter
            cp 1Bh
            jp z,cd_Esc

            ; Прямой выбор A-H/a-h разрешён только для найденного драйвера.
            and   0DFh              ; ASCII: привести букву к верхнему регистру.
            sub   'A'
            cp    8
            jp nc, cd_Loop
            ld    c, a
            call  findDrive
            jp c, cd_Loop
            jp    cd_ReturnDrive

cd_Esc:
            ld    a, 0FFh
            jp ncDialogClose

cd_Enter:
            ld    e, a
            ld    d, 0
            ld    hl, vars.driveList
            add   hl, de
            ld    c, (hl)

cd_ReturnDrive:
            ld    a, c
            jp ncDialogClose

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
; DOS возвращает зарегистрированные драйверы, в том числе без носителя.
; Текущий диск не меняется.
;
; Разрушает: AF, BC, DE, HL.

buildDriveList:
            ld      hl, vars.driveList
            ld      bc, 8
            MXOS_CALL_REGS MXOS_DOS_FILE_GET_DRIVES
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
 ; Drive choice uses the same button template as every other modal.
ncDriveModalOpen:
    ld a,(vars.driveCount)
    dec a
    add a,a
    ld e,a
    ld d,0
    ld hl,ncDriveModalTable
    add hl,de
    ld e,(hl)
    inc hl
    ld d,(hl)
    ex de,hl
    ld (ncDriveModal),hl
    ld a,(vars.driveCount)
    ld b,a
    ld c,0
ncDriveCaption:
    push bc
    ld hl,(ncDriveModal)
    ld a,c
    push bc
    call uiModalButtonAt
    pop bc
    push hl
    ld a,c
    ld e,a
    ld d,0
    ld hl,vars.driveList
    add hl,de
    ld a,(hl)
    add a,a
    ld e,a
    ld hl,ncDriveCaptions
    add hl,de
    ex de,hl
    pop hl
    ld (hl),e
    inc hl
    ld (hl),d
    pop bc
    inc c
    dec b
    jp nz,ncDriveCaption
    ld hl,(ncDriveModal)
    ld a,(vars.driveCount)
    ld c,a
    ld b,UI_FOCUS_LINEAR
    ld a,(vars.chooseDrive)
    call uiModalFocusInit
    ld hl,(ncDriveModal)
    ld a,(vars.driveCount)
    ld b,a
    ld a,(ncFontWidth)
    ld c,0
    push af
    ld a,(state.activePanel)
    ld c,a
    pop af
    call uiModalPanelOpen
    ld hl,aChooseDrive
    jp uiModalTitle
ncDriveModal:       DW 0
ncDriveCaptions:    DB 'A',0,'B',0,'C',0,'D',0,'E',0,'F',0,'G',0,'H',0
ncDriveModalTable:
    DW ncDriveModal1,ncDriveModal2,ncDriveModal3,ncDriveModal4
    DW ncDriveModal5,ncDriveModal6,ncDriveModal7,ncDriveModal8
ncDriveModal1:
    UI_MODAL_COMPACT_BUTTONS_BEGIN 1,1
    UI_MODAL_BUTTON ncDriveCaptions,0,0,0
    UI_MODAL_END
ncDriveModal2:
    UI_MODAL_COMPACT_BUTTONS_BEGIN 1,2
    UI_MODAL_BUTTON ncDriveCaptions,0,0,0
    UI_MODAL_BUTTON ncDriveCaptions,0,1,0
    UI_MODAL_END
ncDriveModal3:
    UI_MODAL_COMPACT_BUTTONS_BEGIN 1,3
    UI_MODAL_BUTTON ncDriveCaptions,0,0,0
    UI_MODAL_BUTTON ncDriveCaptions,0,1,0
    UI_MODAL_BUTTON ncDriveCaptions,0,2,0
    UI_MODAL_END
ncDriveModal4:
    UI_MODAL_COMPACT_BUTTONS_BEGIN 1,4
    UI_MODAL_BUTTON ncDriveCaptions,0,0,0
    UI_MODAL_BUTTON ncDriveCaptions,0,1,0
    UI_MODAL_BUTTON ncDriveCaptions,0,2,0
    UI_MODAL_BUTTON ncDriveCaptions,0,3,0
    UI_MODAL_END
ncDriveModal5:
    UI_MODAL_COMPACT_BUTTONS_BEGIN 2,3
    UI_MODAL_BUTTON ncDriveCaptions,0,0,0
    UI_MODAL_BUTTON ncDriveCaptions,0,1,0
    UI_MODAL_BUTTON ncDriveCaptions,0,2,0
    UI_MODAL_BUTTON ncDriveCaptions,1,0,0
    UI_MODAL_BUTTON ncDriveCaptions,1,1,0
    UI_MODAL_END
ncDriveModal6:
    UI_MODAL_COMPACT_BUTTONS_BEGIN 2,3
    UI_MODAL_BUTTON ncDriveCaptions,0,0,0
    UI_MODAL_BUTTON ncDriveCaptions,0,1,0
    UI_MODAL_BUTTON ncDriveCaptions,0,2,0
    UI_MODAL_BUTTON ncDriveCaptions,1,0,0
    UI_MODAL_BUTTON ncDriveCaptions,1,1,0
    UI_MODAL_BUTTON ncDriveCaptions,1,2,0
    UI_MODAL_END
ncDriveModal7:
    UI_MODAL_COMPACT_BUTTONS_BEGIN 2,4
    UI_MODAL_BUTTON ncDriveCaptions,0,0,0
    UI_MODAL_BUTTON ncDriveCaptions,0,1,0
    UI_MODAL_BUTTON ncDriveCaptions,0,2,0
    UI_MODAL_BUTTON ncDriveCaptions,0,3,0
    UI_MODAL_BUTTON ncDriveCaptions,1,0,0
    UI_MODAL_BUTTON ncDriveCaptions,1,1,0
    UI_MODAL_BUTTON ncDriveCaptions,1,2,0
    UI_MODAL_END
ncDriveModal8:
    UI_MODAL_COMPACT_BUTTONS_BEGIN 2,4
    UI_MODAL_BUTTON ncDriveCaptions,0,0,0
    UI_MODAL_BUTTON ncDriveCaptions,0,1,0
    UI_MODAL_BUTTON ncDriveCaptions,0,2,0
    UI_MODAL_BUTTON ncDriveCaptions,0,3,0
    UI_MODAL_ROW 4
    UI_MODAL_BUTTON ncDriveCaptions,1,0,0
    UI_MODAL_BUTTON ncDriveCaptions,1,1,0
    UI_MODAL_BUTTON ncDriveCaptions,1,2,0
    UI_MODAL_BUTTON ncDriveCaptions,1,3,0
    UI_MODAL_END
