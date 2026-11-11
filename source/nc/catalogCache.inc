; Три общих каталога. Запись: valid, drive, dirLo, dirHi, count, sort, age, pad.
; Панели держат номера слотов и собственные индексы; FILE_INFO не копируются.
; Корень H: закреплён, другой видимый каталог никогда не вытесняется.
ncCatalogInit:
    xor a
    ld (ncCatalogs),a
    ld (ncCatalogs+8),a
    ld (ncCatalogs+16),a
    dec a
    ld (vars.cacheSlotA),a
    ld (vars.cacheSlotB),a
    ret

; A=слот 0..2, HL=метаданные; BC сохранены, DE разрушается.
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
    jp z,ncCatalogFindNext
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
    jp z,ncCatalogFound
ncCatalogFindNext:
    inc c
    ld a,c
    cp 3
    jp c,ncCatalogFind
    call ncCatalogVictim
    ld (ncCatalogChoice),a
    ld c,a
    call ncCatalogDiscard
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
    call ncCatalogTouch
    scf
    ret
ncCatalogFound:
    ld a,c
    ld (ncCatalogChoice),a
    call ncCatalogAssign
    call ncCatalogTouch
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

; Один слот может быть занят другой панелью, ещё один — корнем H:.
; Из оставшихся выбирается свободный либо наиболее давно использованный.
ncCatalogVictim:
    ld hl,vars.cacheSlotB
    ld a,(state.activePanel)
    or a
    jp z,ncCatalogOtherReady
    dec hl
ncCatalogOtherReady:
    ld a,(hl)
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
    cp 7
    jp nz,ncCatalogConsider
    inc hl
    ld a,(hl)
    inc hl
    or (hl)
    jp z,ncCatalogVictimNext
ncCatalogConsider:
    ld a,c
    call ncCatalogEntry
    ld de,6
    add hl,de
    ld a,(ncCatalogBest)
    cp 0FFh
    jp z,ncCatalogChoose
    ld a,(ncCatalogBestAge)
    cp (hl)
    jp nc,ncCatalogVictimNext
ncCatalogChoose:
    ld a,c
    ld (ncCatalogBest),a
    ld a,(hl)
    ld (ncCatalogBestAge),a
ncCatalogVictimNext:
    inc c
    ld a,c
    cp 3
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
    cp 3
    jp c,ncCatalogAgeLoop
    ret

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
    cp 3
    jp c,ncCatalogCloseAgeGap
    pop bc
ncCatalogDiscardData:
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
    cp 3
    jp c,ncCatalogNotifyLoop
    pop hl
    pop de
    pop bc
    pop af
    ret
ncCatalogBases: DW FILE_CACHE_A,FILE_CACHE_B,FILE_CACHE_C
ncCatalogs: BLOCK 3*8,0
ncCatalogDrive: DB 0
ncCatalogDirectory: DW 0
ncCatalogChoice: DB 0
ncCatalogOther: DB 0
ncCatalogBest: DB 0
ncCatalogBestAge: DB 0
ncCatalogTouchAge: DB 0
ncCatalogReadOk: DB 0
ncCatalogNotifyDrive: DB 0
ncCatalogNotifyDirectory: DW 0
ncCatalogNotifyAll: DB 0
