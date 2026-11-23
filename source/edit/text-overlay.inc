ebCheckCacheView:
    ld hl,(v_leftColumn)
    ex de,hl
    ld hl,(ebLeft)
    call compareHLDE
    ex de,hl
    ld (ebLeft),hl
    call nz,ebInvalidateRows
    ld a,(v_leftColumnHi)
    ld hl,ebLeftHigh
    cp (hl)
    ld (hl),a
    jp nz,ebInvalidateRows
    ret

ebPrepareRow:
    push af
    xor a
    ld (ebRowOp),a
    ld a,(editorWholeRow)
    or a
    jp z,ebOrdinaryRow
    ld a,(editorPartialRow)
    or a
    jp nz,ebOrdinaryRow
    ld a,(ebCacheRows)
    or a
    jp z,ebOrdinaryRow
    push hl
    call ebCheckCacheView
    pop de
    call ebFindRow
    jp nc,ebRowHit
    call ebPrepareMiss
    ld a,GFX_TEXT_STORE
    jp ebPrepared
ebRowHit:
    call ebTouchRow
    call ebSelectSlot
    inc hl
    inc hl
    ld e,(hl)
    inc hl
    ld d,(hl)
    inc hl
    ld c,(hl)
    ex de,hl
    ld a,(v_columns)
    sub c
    ld e,a
    ld d,0
    push hl
    ld hl,editorRowBuffer
    add hl,de
    ex de,hl
    pop hl
    ld a,GFX_TEXT_BUFFER
ebPrepared:
    ld (ebRowOp),a
    pop af
    ret
ebOrdinaryRow:
    pop af
    jp editorPrepareRow

;----------------------------------------------------------------------------
; MXOS
;
; Найти готовую строку редактора по адресу начала строки документа.
;
; вход:
;   de = позиция начала строки. Таблицы строк и тегов принадлежат текстовому оверлею.
;
; выход:
;   cf = 0: a = индекс совпавшей строки. cf = 1: готовой строки нет. de сохранён; hl и
;   bc рабочие.
;----------------------------------------------------------------------------

ebFindRow:
    ld a,(ebValid)
    or a
    jp z,ebFindMiss
    call ebHashHead
    ld a,(hl)
ebFindTag:
    inc a
    jp z,ebFindMiss
    dec a
    push af
    call ebTagAddress
    ld a,e
    cp (hl)
    jp nz,ebFindNext
    inc hl
    ld a,d
    cp (hl)
    jp z,ebFindHit
    dec hl
ebFindNext:
    pop af
    ld bc,7
    add hl,bc
    ld a,(hl)
    jp ebFindTag
ebFindMiss:
    scf
    ret
ebFindHit:
    pop af
    or a
    ret

ebHashHead:
    ld a,e
    xor d
    and EDIT_HASH_BYTES-1
    ld l,a
    ld h,EDIT_HASH_HEADS>>8
    ret
ebClearHash:
    ld hl,EDIT_HASH_HEADS
    ld b,EDIT_HASH_BYTES
    ld a,0FFh
ebClearHead:
    ld (hl),a
    inc l
    dec b
    jp nz,ebClearHead
    xor a
    ret

; DE=line start. Prepare a full slice and record the continuation.
ebPrepareMiss:
    push de
    ld a,(ebValid)
    or a
    call z,ebClearHash
    ld hl,ebCacheRows
    cp (hl)
    jp z,ebReplaceRow
    inc a
    ld (ebValid),a
    dec a
    ld (ebSlotIndex),a
    call ebAppendRow
    jp ebMissSlot
ebReplaceRow:
    ld a,(ebOldest)
    call ebTouchRow
    call ebUnlinkHash
ebMissSlot:
    call ebSelectSlot
    pop de
    push hl
    call ebHashHead
    ld c,(hl)
    ld a,(ebNewest)
    ld (hl),a
    ld a,c
    pop hl
    push hl
    ld bc,7
    add hl,bc
    ld (hl),a
    pop hl
    ld (hl),e
    inc hl
    ld (hl),d
    inc hl
    push hl
    ex de,hl
    ld a,(v_columns)
    ld c,a
    xor a
    call editorPrepareRow
    ld a,c
    ex (sp),hl
    pop bc
    ld (hl),c
    inc hl
    ld (hl),b
    inc hl
    ld (hl),a
    ld h,b
    ld l,c
    ld c,a
    ret

ebUnlinkHash:
    call ebTagAddress
    ld e,(hl)
    inc hl
    ld d,(hl)
    ld bc,6
    add hl,bc
    ld a,(hl)
    ld (ebHashNext),a
    call ebHashHead
ebUnlinkProbe:
    ld a,(ebNewest)
    cp (hl)
    jp z,ebUnlinkFound
    ld a,(hl)
    call ebTagAddress
    ld bc,7
    add hl,bc
    jp ebUnlinkProbe
ebUnlinkFound:
    ld a,(ebHashNext)
    ld (hl),a
    ld a,(ebNewest)
    ret

; LRU order runs from oldest to newest. Indices are stable until evicted.
ebTouchRow:
    ld hl,ebNewest
    cp (hl)
    ret z
    ld (ebSlotIndex),a
    call ebTagLinks
    ld d,(hl)
    inc hl
    ld e,(hl)
    ld a,d
    inc a
    jp z,ebMoveOldest
    ld a,d
    call ebTagLinks
    inc hl
    ld (hl),e
    jp ebPatchNewer
ebMoveOldest:
    ld a,e
    ld (ebOldest),a
ebPatchNewer:
    ld a,e
    call ebTagLinks
    ld (hl),d
ebAppendRow:
    ld a,(ebNewest)
    inc a
    jp z,ebFirstRow
    dec a
    call ebTagLinks
    inc hl
    ld a,(ebSlotIndex)
    ld (hl),a
    jp ebSetNewest
ebFirstRow:
    ld a,(ebSlotIndex)
    ld (ebOldest),a
ebSetNewest:
    call ebTagLinks
    ld a,(ebNewest)
    ld (hl),a
    inc hl
    ld (hl),0FFh
    ld a,(ebSlotIndex)
    ld (ebNewest),a
    ret
ebTagLinks:
    call ebTagAddress
    ld bc,5
    add hl,bc
    ret
ebTagAddress:
    ld l,a
    ld h,0
    add hl,hl
    add hl,hl
    add hl,hl
    ld bc,EDIT_TAGS
    add hl,bc
    ret

ebSelectSlot:
    push af
    call ebGlyphOffset
    ld bc,EDIT_MAIN_CACHE
    add hl,bc
    ld (ebSlotAddress),hl
    pop af
    jp ebTagAddress

ebPaintRow:
    ld a,(ebRowOp)
    or a
    jp z,ebPrintLim
    ld (ebQOperation),a
    cp GFX_TEXT_BUFFER
    jp nz,ebPaintNew
    ld a,(editorTextLength)
    or a
    jp z,ebPrintLim            ; The local buffer contains the blank tail.
    jp ebSendRow
ebPaintNew:
    ld a,(editorTextLength)
    or a
    jp z,ebPaintStore
    push bc
    ld c,a
    ld b,0
    ld hl,editorRowBuffer
    add hl,bc
ebTrimTail:
    dec hl
    ld a,(hl)
    cp 32
    jp nz,ebTrimDone
    dec c
    jp nz,ebTrimTail
ebTrimDone:
    ld a,c
    pop bc
ebPaintStore:
    push af
    ld a,(ebCacheStride)
    ld b,a
    pop af
    ld de,editorRowBuffer
ebSendRow:
    ld hl,(ebSlotAddress)
    jp ebQueueCommand

    INCLUDE "hex-text-return.inc"
    INCLUDE "keyboard.inc"
    INCLUDE "text-settings.inc"
    INCLUDE "text-find-entry.inc"
    ASSERT $ <= EDIT_HASH_HEADS
    DS EDIT_HASH_HEADS+EDIT_HASH_BYTES-$,0

; Run one slice after an empty keyboard poll. Scan at most 32 logical bytes
; and prepare at most one visible row. The write goes directly to MAIN.
ebPrefetch:
    ld a,(ebCacheRows)
    cp EDITOR_ROWS+2
    ret c
    ld hl,(v_leftColumn)
    ld a,(v_leftColumnHi)
    or h
    ret nz
    ld a,l
    cp EDIT_PREFETCH_LEFT_LIMIT
    ret nc
    call ebCheckCacheView
    ld hl,(v_topLine)
    ex de,hl
    ld hl,(ebPrefetchTop)
    call compareHLDE
    jp nz,ebPrefetchStart
    ld a,(ebPrefetchPhase)
    or a
    jp z,ebPrefetchStart
    inc a
    ret z
    ld hl,(ebScanPointer)
    ld b,EDIT_SCAN_BYTES
    dec a
    dec a
    jp nz,ebScanBack
ebScanForward:
    call gapTextRead
    or a
    jp z,ebPrefetchBack
    cp 13
    inc hl
    jp nz,ebScanMore
    dec hl
    call editorSkipSeparator
    ld a,(ebPrefetchSkip)
    or a
    jp z,ebPrefetchForwardRow
    dec a
    ld (ebPrefetchSkip),a
    jp z,ebPrefetchForwardRow
ebScanMore:
    dec b
    jp nz,ebScanForward
ebSaveScan:
    ld (ebScanPointer),hl
    ret
ebPrefetchForwardRow:
    ld (ebScanPointer),hl
    call gapTextRead
    or a
    jp z,ebPrefetchBack
    call ebPrefetchRow
    ld hl,ebPrefetchLeft
    dec (hl)
    ret nz
ebPrefetchBack:
    ld a,2
    ld (ebPrefetchPhase),a
    call ebPrefetchLimit
    ld hl,(ebPrefetchTop)
    jp ebStartBackLine
ebPrefetchStart:
    ex de,hl
    ld (ebPrefetchTop),hl
    ld (ebScanPointer),hl
    ld a,EDITOR_ROWS
    ld (ebPrefetchSkip),a
    ld a,1
    ld (ebPrefetchPhase),a
    call ebPrefetchLimit
    ld a,(ebBelowValid)
    or a
    ret z
    ld hl,(ebBelowTop)
    ex de,hl
    ld hl,(ebPrefetchTop)
    call compareHLDE
    ret nz
    xor a
    ld (ebPrefetchSkip),a
    ld hl,(ebBelow)
    jp ebPrefetchForwardRow

; Foreground drawing already knows the first logical row after the viewport.
; Keep that anchor so the idle job can skip traversing the visible page.
ebObserveBottom:
    push af
    ld a,(bios_vars.cursorY)
    cp EDITOR_LAST_Y
    jp nz,ebObserveDone
    ld a,(editorWholeRow)
    or a
    jp z,ebObserveDone
    ld (ebBelow),hl
    push hl
    ld hl,(v_topLine)
    ld (ebBelowTop),hl
    pop hl
    ld (ebBelowValid),a
ebObserveDone:
    pop af
    ret

ebPrefetchLimit:
    ld a,(ebCacheRows)
    ; Leave room for a recently visited screen as well as the visible one.
    cp 3*EDITOR_ROWS
    jp c,ebPrefetchNoReserve
    sub EDITOR_ROWS
ebPrefetchNoReserve:
    sub EDITOR_ROWS
    or a
    rra
    cp 2*EDITOR_ROWS
    jp c,ebPrefetchSetLimit
    ld a,2*EDITOR_ROWS
ebPrefetchSetLimit:
    ld (ebPrefetchLeft),a
    ret

ebStartBackLine:
    push hl
    ld hl,(v_textBeginLo)
    ex de,hl
    pop hl
    call compareHLDE
    jp z,ebPrefetchDone
    call editorBackSeparator
    call compareHLDE
    jp z,ebSaveScan
    dec hl
    jp ebSaveScan
ebScanBack:
    push hl
    ld hl,(v_textBeginLo)
    ex de,hl
    pop hl
ebScanBackByte:
    call compareHLDE
    jp z,ebPrefetchBackRow
    call gapTextRead
    cp 13
    jp z,ebScanAfterCR
    dec hl
    dec b
    jp nz,ebScanBackByte
    jp ebSaveScan
ebScanAfterCR:
    inc hl
ebPrefetchBackRow:
    push hl
    call ebPrefetchRow
    pop hl
    ld a,(ebPrefetchLeft)
    dec a
    ld (ebPrefetchLeft),a
    jp nz,ebStartBackLine
ebPrefetchDone:
    ld a,0FFh
    ld (ebPrefetchPhase),a
    ret

ebPrefetchRow:
    ex de,hl
    call ebFindRow
    ret nc
    call ebPrepareMiss
    ld a,(v_columns)
    sub c
    ld b,a
    ld a,(ebCacheStride)
    sub b
    jp z,ebPrefetchCopy
    ld b,a
    ld a,32
ebPrefetchPad:
    ld (de),a
    inc de
    dec b
    jp nz,ebPrefetchPad
ebPrefetchCopy:
    ld hl,editorRowBuffer
    ld (ebTransfer+1),hl
    ld hl,(ebSlotAddress)
    ld (ebTransfer+4),hl
    ld a,(ebNumber)
    ld (ebTransfer),a
    xor a
    ld (ebTransfer+3),a
    ld a,(ebCacheStride)
    ld l,a
    ld h,0
    ld (ebTransfer+6),hl
    ld hl,ebTransfer
    call mx_memcpyFar
    jp c,ebInvalidateRows
    ld hl,(ebPrefetched)
    inc hl
    ld (ebPrefetched),hl
    ret

    INCLUDE "keyboard-handlers.inc"
    INCLUDE "text-find-match.inc"

