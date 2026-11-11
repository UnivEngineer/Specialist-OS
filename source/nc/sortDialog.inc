; Shift-F1/F2: C=целевая панель. Обычные F1/F2 сохраняют выбор диска.
ncPanelSort:
    ld a,(state.activePanel)
    push af
    ld a,c
    ld (state.activePanel),a
    ld hl,state.panelA_drive
    call readBytePanel
    call loadFiles
    COLOR COLOR_DIALOG
    ld hl,ncSortWindow
    call draw
    ld hl,01860h
    call setCursorPosPanel
    ld hl,ncSortTitle
    call bios_printStringReal
    ld a,' '
    ld (ncSortOptions+1),a
    ld (ncSortOptions+18),a
    ld (ncSortOptions+35),a
    ld hl,state.panelA_sort
    call readBytePanel
    ld (ncSortChoice),a
    call ncSortTextAddress
    inc hl
    ld (hl),'x'
    ld hl,ncSortOptions
    ld b,0
ncSortDrawOption:
    push hl
    ld a,b
    add a,a
    add a,a
    add a,a
    add a,a
    add a,116
    ld l,a
    ld h,24
    call setCursorPosPanel
    pop hl
    call bios_printStringReal
    inc hl
    inc b
    ld a,b
    cp 3
    jp c,ncSortDrawOption
    call ncSortInvert
ncSortKeyLoop:
    call ncModalGetch
    cp 01Bh
    jp z,ncSortClose
    cp 0Dh
    jp z,ncSortAccept
    cp ' '
    jp z,ncSortAccept
    ld b,1
    cp 01Ah
    jp z,ncSortMove
    cp 018h
    jp z,ncSortMove
    cp 9
    jp z,ncSortMove
    ld b,0FFh
    cp 019h
    jp z,ncSortMove
    cp 8
    jp nz,ncSortKeyLoop
ncSortMove:
    ld a,(ncSortChoice)
    add a,b
    cp 0FFh
    jp nz,ncSortWrapNext
    ld a,2
ncSortWrapNext:
    cp 3
    jp c,ncSortMoveReady
    xor a
ncSortMoveReady:
    ld (ncSortNext),a
    call ncSortInvert
    ld a,(ncSortNext)
    ld (ncSortChoice),a
    call ncSortInvert
    jp ncSortKeyLoop
ncSortAccept:
    ld hl,state.panelA_sort
    call readBytePanel
    ld b,a
    ld a,(ncSortChoice)
    cp b
    jp z,ncSortClose
    call writeBytePanel
    ; Сохранить выбранный исходный файл, а не его прежнюю позицию.
    ld hl,state.panelA_curFile
    call readBytePanel
    ld e,a
    ld d,0
    call fileIndexBase
    add hl,de
    ld a,(hl)
    ld (ncSortSelected),a
    call ncCatalogUseIndex
    call ncSortRestoreSelection
ncSortClose:
    ld a,(state.activePanel)
    or a
    ld a,1
    jp z,ncSortDamage
    ld a,2
ncSortDamage:
    ld hl,ncDamageMask
    or (hl)
    ld (hl),a
    pop af
    ld (state.activePanel),a
    ; Информационная панель рисует текущий том файловой панели.
    ; Восстановить его до перерисовки, включая попадание кэша пути.
    ld hl,state.panelA_drive
    call readBytePanel
    call loadFiles
    jp ncRefreshDirty

ncSortRestoreSelection:
    ld hl,state.panelA_filesCnt
    call readBytePanel
    ld b,a
    call fileIndexBase
    ld c,0
    ld a,b
    or a
    jp z,ncSortSelectionReady
ncSortFindSelected:
    ld a,(ncSortSelected)
    cp (hl)
    jp z,ncSortSelectionFound
    inc hl
    inc c
    dec b
    jp nz,ncSortFindSelected
    ld c,0
ncSortSelectionFound:
    ld a,c
ncSortSelectionReady:
    ld hl,state.panelA_curFile
    call writeBytePanel
    sub FILE_LIST_SIZE
    jp nc,ncSortSelectionScroll
    ld a,0FFh
ncSortSelectionScroll:
    inc a
    ld hl,state.panelA_firstFile
    jp writeBytePanel

ncSortInvert:
    ld a,(ncSortChoice)
    add a,a
    add a,a
    add a,a
    add a,a
    add a,108
    ld l,a
    ld h,096h
    call activePanelPos
    ld bc,(12<<8)+11
    jp inverseRect
ncSortTextAddress:
    ld e,a
    ld d,0
    ld l,a
    ld h,0
    add hl,hl
    add hl,hl
    add hl,hl
    add hl,hl
    add hl,de
    ld de,ncSortOptions
    add hl,de
    ret
ncSortWindow:
    G_WINDOW 0,32,80,128,78
    G_LINE 0,40,102,112
    DB 0
ncSortTitle: DB 'Sort panel      ',0
ncSortOptions:
    DB '[ ] Unsorted    ',0
    DB '[ ] Name        ',0
    DB '[ ] Extension   ',0
    ASSERT $-ncSortOptions == 3*17
ncSortChoice: DB 0
ncSortNext: DB 0
ncSortSelected: DB 0
