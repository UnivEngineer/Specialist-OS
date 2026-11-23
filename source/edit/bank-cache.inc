; EDITCORE owns the row metadata and registers the glyph storage in MAIN.
; Tags contain line DW, continuation DW, unused width, older/newer indices.
; The last byte links entries with the same hash.
EDIT_SCAN_BYTES          = 32
EDIT_PREFETCH_LEFT_LIMIT = 96

;----------------------------------------------------------------------------
; MXOS
;
; Зарегистрировать графическую область и кэш строк EDITCORE в MAIN.
;
; вход:
;   UI_GFX загружен в MAIN; ширина шрифта доступна по EDIT_MAIN_FONT_WIDTH.
;
; выход:
;   cf = 0 при успехе, ebCacheStride и ebCacheRows заданы. При нехватке места кэш может
;   иметь нулевую ёмкость. При отказе обязательной регистрации cf = 1, a = код.
;----------------------------------------------------------------------------

ebRegisterWorkspace:
    ld hl,EDIT_MAIN_FONT_WIDTH
    ld b,0
    ld a,MEM_OP_GET8
    call mx_memory
    ret c
    ld a,e
    ld c,96
    cp 4
    jp z,ebSetStride
    ld c,64
    cp 6
    jp z,ebSetStride
    ld c,48
ebSetStride:
    ld a,c
    ld (ebCacheStride),a
    ld bc,MEM_RESIDENT_FONT
    ld a,MEM_OP_RESIDENT
    MXOS_CALL MXOS_DOS_MEMORY
    jp c,ebWorkspaceOnly
    ld a,h
    or l
    jp nz,ebCacheCeilingReady
    ld hl,APP_MEMORY_END
ebCacheCeilingReady:
    ld de,EDIT_MAIN_CACHE
    call ebSubtract
    jp c,ebWorkspaceOnly
    ld a,(ebCacheStride)
    ld e,a
    ld d,0
    ld b,0
ebCountSlots:
    call ebSubtract
    jp c,ebRegisterSlots
    inc b
    ld a,b
    cp EDIT_CACHE_MAX_ROWS
    jp nz,ebCountSlots
ebRegisterSlots:
    ld a,b
    ld (ebCacheRows),a
    call ebGlyphOffset
    ld de,EDIT_MAIN_END-GFX_PACKET
    add hl,de
    ex de,hl
    ld hl,GFX_PACKET
    ld b,0
    ld a,MEM_OP_MAINREGISTER
    MXOS_CALL MXOS_DOS_MEMORY
    ret nc
    push af
    ld a,(ebCacheRows)
    or a
    jp z,ebWorkspaceFailed
    pop de
    sub 16                     ; A foreign allocation may cap this range.
    jp nc,ebTrySmaller
ebWorkspaceOnly:
    xor a
ebTrySmaller:
    ld b,a
    jp ebRegisterSlots
ebWorkspaceFailed:
    pop af
    ret

;----------------------------------------------------------------------------
; MXOS
;
; Сбросить кэш подготовленных строк и фоновую подготовку в банковом редакторе.
;
; вход:
;   Рабочие переменные оверлея доступны в текущей странице.
;
; выход:
;   ebValid, ebPrefetchPhase и ebBelowValid обнулены. Голова и хвост списка LRU равны
;   0FFh. Регистры и флаги сохранены.
;----------------------------------------------------------------------------

ebInvalidateRows:
    push af
    xor a
    ld (ebValid),a
    ld (ebPrefetchPhase),a
    ld (ebBelowValid),a
    dec a
    ld (ebOldest),a
    ld (ebNewest),a
    pop af
    ret

; A=index or capacity; HL=A * stride for the active font.
ebGlyphOffset:
    ld l,a
    ld h,0
    DUP 4
        add hl,hl
    EDUP
    ld a,(ebCacheStride)
    cp 48
    jp z,ebTimesThree
    add hl,hl
    cp 96
    jp z,ebTimesThree
    add hl,hl
    ret
ebTimesThree:
    ld b,h
    ld c,l
    add hl,hl
    add hl,bc
    ret

ebCacheRows:        DB 0
ebCacheStride:      DB 0
ebValid:            DB 0
ebOldest:           DB 0FFh
ebNewest:           DB 0FFh
ebSlotIndex:        DB 0
ebHashNext:         DB 0
ebLeft:             DW 0
ebLeftHigh:         DB 0
ebRowOp:            DB 0
ebSlotAddress:      DW 0
ebPrefetchPhase:    DB 0    ; 0=rebase, 1=forward, 2=backward, FF=done
ebPrefetchSkip:     DB 0
ebPrefetchLeft:     DB 0
ebPrefetchTop:      DW 0
ebScanPointer:      DW 0
ebBelow:            DW 0
ebBelowTop:         DW 0
ebBelowValid:       DB 0
ebPrefetched:       DW 0
