; Каталог панели кэшируется целиком до предела 8-разрядной навигации.
; Вход A=диск, активная панель выбирает слот общего кэша. AF/BC/DE/HL разрушены.
; На попадании кэша дисковый список не читается; выбирается окно индексов.
; Готовый индекс возвращается вместе с каталогом, сортировка — только при miss режима.
loadFiles:
            ld      c, a
            ld      e, 1
            MXOS_CALL MXOS_DOS_FILE_SELECT_DRIVE
            push    bc
            call    restorePanelDirectory
            pop     bc
            call    ncCatalogAcquire
            jp c,   lf_ReadCatalog
            call    ncCatalogUseIndex
            jp      lf_Cached

lf_ReadCatalog:
            ld a,1
            ld (ncCatalogReadOk),a
            call    fileCacheBase
            push hl
            ld hl,state.panelA_dirLow
            call readBytePanel
            ld c,a
            ld hl,state.panelA_dirHigh
            call readBytePanel
            or c
            ld bc,0
            jp z,lf_ListFirstReady
            inc bc ; Skip FAT "." in subdirectories; keep "..".
lf_ListFirstReady:
            pop hl
            ld a,(ncCatalogCapacity)
            ld e,a
            ld d,0
            ld      a, DIR_OP_LIST
            MXOS_CALL MXOS_DOS_DIRECTORY
            jp c,lf_ListFailed
            ld a,b
            or c
            jp z,lf_ListReady
            dec de ; DOS total includes the skipped "." record.
            jp lf_ListReady
lf_ListFailed:
            xor a
            ld (ncCatalogReadOk),a
            ld de,0
            call fileCacheBase
            ld (hl),0FFh
lf_ListReady:
            ; Future optional "Go to drive root" entry: request BC=0 above
            ; and re-enable this conversion of FAT "." to the UI-only "\".
            ; push hl
            ; push de
            ; call fileCacheBase
            ; ld a,(hl)
            ; cp '.'
            ; jp nz,lf_RootReady
            ; inc hl
            ; ld a,(hl)
            ; cp ' '
            ; dec hl
            ; jp nz,lf_RootReady
            ; ld (hl),05Ch
            ; lf_RootReady:
            ; pop de
            ; pop hl

lf_ClampCount:
            ; DOS returns total count, even when only capacity records fit.
            ; Clamp to the runtime capacity before building byte indices.
            ld a,(ncCatalogCapacity)
            ld c,a
            ld a,d
            or a
            jp nz,lf_CapCount
            ld a,e
            cp c
            jp c,lf_StoreCount
lf_CapCount:
            ld a,c
lf_StoreCount:
            ld      hl, state.panelA_filesCnt
            call    writeBytePanel
            ld a,(ncCatalogReadOk)
            call ncCatalogCommit

lf_Cached:
            call ncStateSelect
            call ncParentSelect
            ; Удаление могло сократить каталог. Пустая панель имеет позиции 0.
            ld      hl, state.panelA_filesCnt
            call    readBytePanel
            ld      c, a
            or      a
            jp z,   lf_StoreCursor
            dec     a
            ld      b, a
            ld      hl, state.panelA_curFile
            call    readBytePanel
            cp      c
            jp c,   lf_CursorReady
            ld      a, b
lf_StoreCursor:
            ld      hl, state.panelA_curFile
            call    writeBytePanel
lf_CursorReady:
            ld      c, a
            ld      hl, state.panelA_firstFile
            call    readBytePanel
            cp      c
            jp c,   lf_View
            jp z,   lf_View
            ld      a, c
            call    writeBytePanel

lf_View:
            ld b,a
            ld a,c
            sub b
            call ncSubVisible
            ld a,b
            jp c,lf_ViewReady
            ld a,c
            call ncSubVisible
            inc a
            call writeBytePanel
lf_ViewReady:
            ; Видимое окно состоит из байтовых индексов, без копирования записей.
            ld e,a
            ld d,0
            call fileIndexBase
            add hl,de
            ld (vars.fileListPtr),hl
            ret

; HL=общий каталог, назначенный активной панели; BC/DE сохранены.
fileCacheBase:
            ld hl,vars.cacheSlotA
            call readBytePanel
            jp ncCatalogBase

; Явное обновление отменяет и общий каталог, и связанные панельные индексы.
invalidateFileCache:
            push bc
            push de
            ld hl,vars.cacheSlotA
            call readBytePanel
            cp 0FFh
            jp z,lf_NoSlot
            ld c,a
            call ncCatalogDiscard
lf_NoSlot:
            pop de
            pop bc
            ret
