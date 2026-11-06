; Общая маска состояния: биты 0/1 — левая/правая панель.
; Изменения каталога требуют перечитать кэш; следы диалога — только рисовать.
; Наблюдатель DOS не вызывает BIOS: A=диск, DE=каталог, C=0/1 точный/весь диск.
ncDiskChanged:
    ld b,a
    ld a,(state.panelA_drive)
    cp b
    jp nz,ncChangedRight
    ld a,(vars.panelA_info)
    or c
    jp nz,ncChangedLeftMatch
    ld a,(state.panelA_dirLow)
    cp e
    jp nz,ncChangedRight
    ld a,(state.panelA_dirHigh)
    cp d
    jp nz,ncChangedRight
ncChangedLeftMatch:
    ld a,(ncDirtyMask)
    or 1
    ld (ncDirtyMask),a
ncChangedRight:
    ld a,(state.panelB_drive)
    cp b
    ret nz
    ld a,(vars.panelB_info)
    or c
    jp nz,ncChangedRightMatch
    ld a,(state.panelB_dirLow)
    cp e
    ret nz
    ld a,(state.panelB_dirHigh)
    cp d
    ret nz
ncChangedRightMatch:
    ld a,(ncDirtyMask)
    or 2
    ld (ncDirtyMask),a
    ret
ncStopObserver:
    ld hl,0
    ld a,CLI_OP_OBSERVER
    MXCALL MX_DOS_CLI
    ret
; После тихой команды: приглашение обновляется сразу, панели — по маске.
ncRefreshDirty:
    call updateCommandPath
    call clearCmdLine
    ld a,1
    ld (ncSelectiveRefresh),a
    ld a,(ncDirtyMask)
    and 1
    jp z,ncDirtyRightCache
    xor a
    ld (vars.cacheValidA),a
ncDirtyRightCache:
    ld a,(ncDirtyMask)
    and 2
    jp z,ncDirtyDraw
    xor a
    ld (vars.cacheValidB),a
ncDirtyDraw:
    ld a,(ncDamageMask)
    or a
    ld a,0
    jp nz,ncDirtyFrames
    inc a
ncDirtyFrames:
    ld (ncKeepFrames),a
    ld a,(ncDirtyMask)
    ld b,a
    ld a,(ncDamageMask)
    or b
    ld (ncRefreshMask),a
    and 1
    call nz,ncRefreshLeft
    ld a,(ncRefreshMask)
    and 2
    call nz,ncRefreshRight
    xor a
    ld (ncDirtyMask),a
    ld (ncDamageMask),a
    ld (ncSelectiveRefresh),a
    ld (ncKeepFrames),a
    call updateCommandPath
    jp mainLoop
ncDamageBoth:
    ld a,3
    ld (ncDamageMask),a
    ret
ncDirtyMask: DB 0
ncDamageMask: DB 0
ncSelectiveRefresh: DB 0
ncRefreshMask: DB 0

; Строка сведений также принадлежит обновляемой панели. Кэш другой панели
; используется только для восстановления указателя выделения, без рисования.
ncRefreshLeft:
    call loadAndPrintA
    ld c,0
    jp ncRefreshInfo
ncRefreshRight:
    call loadAndPrintB
    ld c,1
ncRefreshInfo:
    ld a,(state.activePanel)
    push af
    ld a,c
    ld (state.activePanel),a
    ld hl,state.panelA_info
    call readBytePanel
    or a
    jp nz,ncRefreshInfoRestore
    ld hl,state.panelA_drive
    call readBytePanel
    call loadFiles
    call printInfoLine
ncRefreshInfoRestore:
    pop af
    ld (state.activePanel),a
    ld hl,state.panelA_drive
    call readBytePanel
    jp loadFiles
