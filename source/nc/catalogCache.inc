; Метаданные слотов: valid, drive, dirLo, dirHi, count, sort, age, overflow.
; overflow: 0 = full list, 1 = warning pending, 2 = warning acknowledged.
CATALOG_OVERFLOW = 7
; Панели держат номера слотов и собственные индексы; FILE_INFO не копируются.
; Видимый каталог другой панели не вытесняется.
ncCatalogInit:
    xor a
    ld hl,ncCatalogs
    ld b,NC_CATALOG_SLOTS*8
ncCatalogClear:
    ld (hl),a
    inc hl
    dec b
    jp nz,ncCatalogClear
    ld (ncCatalogWarnPending),a
    dec a
    ld (vars.cacheSlotA),a
    ld (vars.cacheSlotB),a
    ret

; A=номер слота, HL=метаданные; BC сохранены, DE разрушается.

;----------------------------------------------------------------------------
; MXOS
;
; Найти запись каталога панели в таблице NC.
;
; вход:
;   a = индекс каталога. Индекс должен находиться в границах таблицы.
;
; выход:
;   hl = ncCatalogs + 8 × индекс, de = ncCatalogs. a и bc сохранены, cf определяется
;   сложением адресов.
;----------------------------------------------------------------------------

ncCatalogEntry:
    ld l,a
    ld h,0
    add hl,hl
    add hl,hl
    add hl,hl
    ld de,ncCatalogs
    add hl,de
    ret

; A=слот, HL=физический список; BC/DE сохранены.

;----------------------------------------------------------------------------
; MXOS
;
; Прочитать базовый адрес буфера записей каталога.
;
; вход:
;   a = индекс каталога в ncCatalogBases.
;
; выход:
;   hl = 16-битный адрес буфера, a = его младший байт. bc и de сохранены.
;----------------------------------------------------------------------------

ncCatalogBase:
    push de
    add a,a
    ld e,a
    ld d,0
    ld hl,ncCatalogBases
    add hl,de
    ld a,(hl)
    inc hl
    ld h,(hl)
    ld l,a
    pop de
    ret
ncCatalogIndexBase:
    ld hl,CATALOG_INDEX_A
    add a,h
    ld h,a
    ret

; C=диск. CF=0: каталог уже есть, CF=1: слот назначен для чтения с диска.
; Активная панель задаёт кластер каталога; AF/BC/DE/HL разрушаются.
ncCatalogAcquire:
    ld a,c
    ld (ncCatalogDrive),a
    ld hl,state.panelA_dirLow
    call readBytePanel
    ld (ncCatalogDirectory),a
    ld hl,state.panelA_dirHigh
    call readBytePanel
    ld (ncCatalogDirectory+1),a
    ld c,0
ncCatalogFind:
    ld a,c
    call ncCatalogEntry
    ld a,(hl)
    or a
    jp nz,ncCatalogMatchKey
    ; A filesystem notification invalidates the records, but a matching
    ; truncated directory can reuse the slot and its warning state.
    push hl
    ld de,CATALOG_OVERFLOW
    add hl,de
    ld a,(hl)
    pop hl
    or a
    jp z,ncCatalogFindNext
ncCatalogMatchKey:
    inc hl
    ld a,(ncCatalogDrive)
    cp (hl)
    jp nz,ncCatalogFindNext
    inc hl
    ld a,(ncCatalogDirectory)
    cp (hl)
    jp nz,ncCatalogFindNext
    inc hl
    ld a,(ncCatalogDirectory+1)
    cp (hl)
    jp z,ncCatalogMatch
ncCatalogFindNext:
    inc c
    ld a,c
    cp NC_CATALOG_SLOTS
    jp c,ncCatalogFind
    call ncCatalogVictim
    ld (ncCatalogChoice),a
    ld c,a
    call ncCatalogDiscard
    ld a,(ncCatalogChoice)
    call ncCatalogEntry
    ld de,CATALOG_OVERFLOW
    add hl,de
    ld (hl),0
    jp ncCatalogStartRead
ncCatalogMatch:
    ld a,c
    ld (ncCatalogChoice),a
    call ncCatalogEntry
    ld a,(hl)
    or a
    jp nz,ncCatalogFound
ncCatalogStartRead:
    ; Новый список в том же физическом слоте тоже отменяет индекс панели.
    xor a
    ld hl,vars.cacheValidA
    call writeBytePanel
    ld a,(ncCatalogChoice)
    call ncCatalogEntry
    ld (hl),0
    inc hl
    ld a,(ncCatalogDrive)
    ld (hl),a
    inc hl
    ld a,(ncCatalogDirectory)
    ld (hl),a
    inc hl
    ld a,(ncCatalogDirectory+1)
    ld (hl),a
    inc hl
    ld (hl),0
    inc hl
    ld (hl),0FFh
    inc hl
    ld (hl),2
    call ncCatalogAssign
    IF NC_BANKED
    call ncCatalogTouch
    ELSE
    call ncCatalogPlace
    ENDIF
    scf
    ret
ncCatalogFound:
    IF !NC_BANKED
    ; A previously clipped list can grow on the next load of this panel.
    push hl
    ld de,CATALOG_OVERFLOW
    add hl,de
    ld a,(hl)
    pop hl
    or a
    jp z,ncCatalogFoundReady
    call ncCatalogOtherSlot
    ld b,a
    ld a,(ncCatalogChoice)
    cp b
    jp z,ncCatalogFoundReady
    call ncCatalogRoom
    push af
    ld a,(ncCatalogChoice)
    call ncCatalogEntry
    ld de,4
    add hl,de
    pop af
    cp (hl)
    jp c,ncCatalogFoundReady
    jp nz,ncCatalogStartRead
ncCatalogFoundReady:
    ENDIF
    call ncCatalogAssign
    IF NC_BANKED
    call ncCatalogTouch
    ENDIF
    ld a,(ncCatalogChoice)
    call ncCatalogEntry
    ld de,4
    add hl,de
    ld a,(hl)
    ld hl,state.panelA_filesCnt
    call writeBytePanel
    or a
    ret

ncCatalogAssign:
    ld hl,vars.cacheSlotA
    call readBytePanel
    ld b,a
    ld a,(ncCatalogChoice)
    cp b
    call writeBytePanel
    ret z
    xor a
    ld hl,vars.cacheValidA
    jp writeBytePanel

; Slot associated with the other visible panel (FFh before its first read).
ncCatalogOtherSlot:
    ld hl,vars.cacheSlotB
    ld a,(state.activePanel)
    or a
    jp z,ncCatalogOtherReady
    dec hl
ncCatalogOtherReady:
    ld a,(hl)
    ret

ncCatalogVictim:
    call ncCatalogOtherSlot
    IF !NC_BANKED
    cp NC_CATALOG_SLOTS
    jp c,ncCatalogOpposite
    xor a
    ret
ncCatalogOpposite:
    xor 1
    ret
    ELSE
    ; Prefer to retain roots of the two panel drives, with LRU in each group.
    ld (ncCatalogOther),a
    ld a,0FFh
    ld (ncCatalogBest),a
    xor a
    ld (ncCatalogBestAge),a
    ld c,0
ncCatalogVictimLoop:
    ld a,(ncCatalogOther)
    cp c
    jp z,ncCatalogVictimNext
    ld a,c
    call ncCatalogEntry
    ld a,(hl)
    or a
    ld a,c
    ret z
    inc hl
    ld a,(hl)
    ld b,a
    inc hl
    ld a,(hl)
    inc hl
    or (hl)
    ld e,0
    jp nz,ncCatalogScore
    ld a,(state.panelA_drive)
    cp b
    jp z,ncCatalogPreferredRoot
    ld a,(state.panelB_drive)
    cp b
    jp nz,ncCatalogScore
ncCatalogPreferredRoot:
    ld e,NC_CATALOG_SLOTS
ncCatalogScore:
    ; HL points at directory high. Age is three bytes further.
    inc hl
    inc hl
    inc hl
    ld a,(hl)
    sub e
    add a,NC_CATALOG_SLOTS
    ld b,a
    ld a,(ncCatalogBest)
    cp 0FFh
    jp z,ncCatalogChoose
    ld a,(ncCatalogBestAge)
    cp b
    jp nc,ncCatalogVictimNext
ncCatalogChoose:
    ld a,c
    ld (ncCatalogBest),a
    ld a,b
    ld (ncCatalogBestAge),a
ncCatalogVictimNext:
    inc c
    ld a,c
    cp NC_CATALOG_SLOTS
    jp c,ncCatalogVictimLoop
    ld a,(ncCatalogBest)
    ret
ncCatalogTouch:
    ld a,(ncCatalogChoice)
    call ncCatalogEntry
    ld de,6
    add hl,de
    ld a,(hl)
    ld (ncCatalogTouchAge),a
    ld c,0
ncCatalogAgeLoop:
    ld a,c
    call ncCatalogEntry
    ld de,6
    add hl,de
    ld a,(ncCatalogChoice)
    cp c
    jp nz,ncCatalogAgeOther
    ld (hl),0
    jp ncCatalogAgeNext
ncCatalogAgeOther:
    ld a,(ncCatalogTouchAge)
    cp (hl)
    jp c,ncCatalogAgeNext
    jp z,ncCatalogAgeNext
    inc (hl)
ncCatalogAgeNext:
    inc c
    ld a,c
    cp NC_CATALOG_SLOTS
    jp c,ncCatalogAgeLoop
    ret

    ENDIF

    IF !NC_BANKED
; Report free records for the chosen slot while reserving the other list.
; DE = bytes to preserve (zero when there is no distinct list to move).
ncCatalogRoom:
    call ncCatalogOtherSlot
    ld de,0
    cp NC_CATALOG_SLOTS
    jp nc,ncCatalogRoomReserve
    ld b,a
    ld a,(ncCatalogChoice)
    cp b
    jp z,ncCatalogRoomReserve
    ld a,b
    call ncCatalogEntry
    ld de,4
    add hl,de
    ld l,(hl)             ; Include the other list's sentinel.
    ld h,0
    inc hl
    DUP 4
        add hl,hl
    EDUP
    ex de,hl
    push de
    ld hl,(NC_MIN_CATALOG_CAPACITY+1)*FILE_INFO_SIZE
    call bios_cmp_hl_de
    jp c,ncCatalogRoomCompute
    ex de,hl
ncCatalogRoomCompute:
    call ncCatalogFreeCount
    pop de
    ret
ncCatalogRoomReserve:
    ; Give the second panel a full column at initialization.
    ld de,(NC_MIN_CATALOG_CAPACITY+1)*FILE_INFO_SIZE
    call ncCatalogFreeCount
    ld de,0
    ret
ncCatalogFreeCount:
    ld hl,(ncCatalogPoolSize)
    call bios_sub_hl_de
    ld bc,FILE_INFO_SIZE
    ld a,l
    sub c
    ld l,a
    jp nc,ncCatalogCountShift
    dec h
ncCatalogCountShift:
    ld a,h
    cp 16
    ld a,255
    ret nc
    ld a,l
    DUP 4
        rrca
    EDUP
    and 0Fh
    ld c,a
    ld a,h
    DUP 4
        rlca
    EDUP
    or c
    ret

; Compact the preserved list at the bottom, then read the replacement
; directly into the contiguous tail. Relocating FILE_INFO preserves indices.
ncCatalogPlace:
    call ncCatalogRoom
    ld (ncCatalogCapacity),a
    ld hl,FILE_CACHE_A
    ld a,d
    or e
    jp z,ncCatalogSetBase
    add hl,de
    push hl                 ; end of the preserved list at its destination
    call ncCatalogOtherSlot
    push af
    call ncCatalogBase
    ld bc,FILE_CACHE_A
    ld a,h
    cp b
    jp nz,ncCatalogMoveOther
    ld a,l
    cp c
    jp z,ncCatalogOtherPlaced
ncCatalogMoveOther:
    push hl
    add hl,de
    ex de,hl
    pop hl
    call bios_memcpy_bc_hl
ncCatalogOtherPlaced:
    pop af
    ld hl,FILE_CACHE_A
    call ncCatalogStoreBase
    pop hl
ncCatalogSetBase:
    ld a,(ncCatalogChoice)
    ; Fall through to write the base for this physical slot.

;----------------------------------------------------------------------------
; MXOS
;
; Назначить буфер записей выбранному каталогу.
;
; вход:
;   a = индекс каталога, hl = базовый адрес буфера.
;
; выход:
;   Адрес сохранён в ncCatalogBases в порядке от младшего байта к старшему. de =
;   исходный адрес, hl указывает на старший байт ячейки. af и bc изменяются.
;----------------------------------------------------------------------------

ncCatalogStoreBase:
    add a,a
    ld e,a
    ld d,0
    ex de,hl
    ld bc,ncCatalogBases
    add hl,bc
    ld (hl),e
    inc hl
    ld (hl),d
    ret
    ENDIF

; Завершение чтения: A=1 при успехе, 0 при ошибке (ошибка не кэшируется).
ncCatalogCommit:
    push af
    ld hl,vars.cacheSlotA
    call readBytePanel
    call ncCatalogEntry
    pop af
    ld (hl),a
    ld de,4
    add hl,de
    push hl
    ld hl,state.panelA_filesCnt
    call readBytePanel
    pop hl
    ld (hl),a
    call buildFileIndex
    jp ncCatalogSaveIndex

; Индекс нужного режима уже у панели, сохранён в слоте либо строится впервые.
ncCatalogUseIndex:
    ld hl,vars.cacheValidA
    call readBytePanel
    or a
    jp z,ncCatalogRestoreIndex
    ld hl,vars.cacheSortA
    call readBytePanel
    ld b,a
    ld hl,state.panelA_sort
    call readBytePanel
    cp b
    ret z
ncCatalogRestoreIndex:
    ld hl,vars.cacheSlotA
    call readBytePanel
    ld (ncCatalogChoice),a
    call ncCatalogEntry
    ld de,5
    add hl,de
    ld b,(hl)
    ld hl,state.panelA_sort
    call readBytePanel
    cp b
    jp nz,ncCatalogBuildIndex
    call fileIndexBase
    ld b,h
    ld c,l
    ld a,(ncCatalogChoice)
    call ncCatalogIndexBase
    ld d,h
    ld e,l
    inc d
    call bios_memcpy_bc_hl
    jp ncCatalogIndexReady
ncCatalogBuildIndex:
    call buildFileIndex
ncCatalogSaveIndex:
    ld hl,vars.cacheSlotA
    call readBytePanel
    ld (ncCatalogChoice),a
    call ncCatalogIndexBase
    ld b,h
    ld c,l
    call fileIndexBase
    ld d,h
    ld e,l
    inc d
    call bios_memcpy_bc_hl
    ld a,(ncCatalogChoice)
    call ncCatalogEntry
    ld de,5
    add hl,de
    push hl
    ld hl,state.panelA_sort
    call readBytePanel
    pop hl
    ld (hl),a
ncCatalogIndexReady:
    ld hl,state.panelA_sort
    call readBytePanel
    ld hl,vars.cacheSortA
    call writeBytePanel
    ld a,(ncCatalogChoice)
    call ncCatalogEntry
    ld a,(hl)
    ld hl,vars.cacheValidA
    jp writeBytePanel

; Отмена слота C и обоих панельных индексов, если они ссылаются на него.
ncCatalogDiscard:
    IF NC_BANKED
    ; Удаление любого ранга сжимает оставшиеся: 0 — самый свежий.
    ; Это сохраняет точный LRU и после уведомлений, и при замене слота.
    ld a,c
    call ncCatalogEntry
    ld a,(hl)
    or a
    jp z,ncCatalogDiscardData
    ld de,6
    add hl,de
    ld b,(hl)
    push bc
    ld c,0
ncCatalogCloseAgeGap:
    ld a,c
    call ncCatalogEntry
    ld a,(hl)
    or a
    jp z,ncCatalogCloseAgeNext
    ld de,6
    add hl,de
    ld a,b
    cp (hl)
    jp nc,ncCatalogCloseAgeNext
    dec (hl)
ncCatalogCloseAgeNext:
    inc c
    ld a,c
    cp NC_CATALOG_SLOTS
    jp c,ncCatalogCloseAgeGap
    pop bc
ncCatalogDiscardData:
    ENDIF
    ld a,c
    call ncCatalogEntry
    ld (hl),0
    ld de,5
    add hl,de
    ld (hl),0FFh
    ld a,(vars.cacheSlotA)
    cp c
    jp nz,ncCatalogDiscardRight
    xor a
    ld (vars.cacheValidA),a
ncCatalogDiscardRight:
    ld a,(vars.cacheSlotB)
    cp c
    ret nz
    xor a
    ld (vars.cacheValidB),a
    ret

; Наблюдатель DOS: A=диск, DE=каталог, C=0/1 точный/весь том.
; Сохраняет все регистры. DOS/BIOS из обработчика не вызываются.
ncCatalogChanged:
    push af
    push bc
    push de
    push hl
    ld (ncCatalogNotifyDrive),a
    ld a,c
    ld (ncCatalogNotifyAll),a
    ex de,hl
    ld (ncCatalogNotifyDirectory),hl
    ld c,0
ncCatalogNotifyLoop:
    ld a,c
    call ncCatalogEntry
    ld a,(hl)
    or a
    jp z,ncCatalogNotifyNext
    inc hl
    ld a,(ncCatalogNotifyDrive)
    cp (hl)
    jp nz,ncCatalogNotifyNext
    ld a,(ncCatalogNotifyAll)
    or a
    jp nz,ncCatalogNotifyMatch
    inc hl
    ld a,(ncCatalogNotifyDirectory)
    cp (hl)
    jp nz,ncCatalogNotifyNext
    inc hl
    ld a,(ncCatalogNotifyDirectory+1)
    cp (hl)
    jp nz,ncCatalogNotifyNext
ncCatalogNotifyMatch:
    call ncCatalogDiscard
ncCatalogNotifyNext:
    inc c
    ld a,c
    cp NC_CATALOG_SLOTS
    jp c,ncCatalogNotifyLoop
    pop hl
    pop de
    pop bc
    pop af
    ret

; After a disk read, HL points to the assigned slot's overflow byte.
; The slot is assigned even if the read failed. BC preserved.
ncCatalogActiveOverflow:
    ld hl,vars.cacheSlotA
    call readBytePanel
    call ncCatalogEntry
    ld de,CATALOG_OVERFLOW
    add hl,de
    ret

; DE=total size of the just-read listing. BC preserved.
ncCatalogRememberTotal:
    push de
    ld hl,vars.cacheSlotA
    call readBytePanel
    call ncCatalogTotalAddress
    pop de
    ld (hl),e
    inc hl
    ld (hl),d
    ret

; A=slot, HL=address of its 16-bit total. DE destroyed.

;----------------------------------------------------------------------------
; MXOS
;
; Найти счётчик общего числа записей каталога.
;
; вход:
;   a = индекс каталога в ncCatalogTotals.
;
; выход:
;   hl = адрес 16-битного счётчика, de = смещение записи. a и флаги изменяются, bc
;   сохранён.
;----------------------------------------------------------------------------

ncCatalogTotalAddress:
    add a,a
    ld e,a
    ld d,0
    ld hl,ncCatalogTotals
    add hl,de
    ret

; A=panel. Return overflow state (zero for no valid slot), HL=flag and C=slot
; if nonzero. BC is otherwise unspecified.
ncCatalogPanelOverflow:
    ld e,a
    ld d,0
    ld hl,vars.cacheSlotA
    add hl,de
    ld a,(hl)
    cp NC_CATALOG_SLOTS
    jp nc,ncCatalogNoOverflow
    ld c,a
    call ncCatalogEntry
    ld a,(hl)
    or a
    ret z
    ld de,CATALOG_OVERFLOW
    add hl,de
    ld a,(hl)
    ret
ncCatalogNoOverflow:
    xor a
    ret
ncCatalogBases:
    IF NC_BANKED
    DW FILE_CACHE_A,FILE_CACHE_B,FILE_CACHE_C,FILE_CACHE_C+FILE_CACHE_BYTES
    ELSE
    DW FILE_CACHE_A,FILE_CACHE_A
    ENDIF
ncCatalogs:             BLOCK  NC_CATALOG_SLOTS*8,0
ncCatalogTotals:        BLOCK  NC_CATALOG_SLOTS*2,0     ; Only used for a valid truncated listing.
ncCatalogDrive:         DB     0
ncCatalogDirectory:     DW     0
ncCatalogChoice:        DB     0
    IF NC_BANKED
ncCatalogOther:     DB 0
ncCatalogBest:      DB 0
ncCatalogBestAge:   DB 0
ncCatalogTouchAge:  DB 0
    ENDIF
ncCatalogReadOk:            DB 0
ncCatalogNotifyDrive:       DB 0
ncCatalogNotifyDirectory:   DW 0
ncCatalogNotifyAll:         DB 0
