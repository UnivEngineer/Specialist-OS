; Contiguous rows share one position and one graphics record.
hexPacketEnable:
    xor a
    ld (hexPacketActive),a
    ld hl,(v_selectionBegin)
    ld a,h
    or l
    ret nz
    inc a
    ld (hexPacketActive),a
    ret

; CF when the current row has already been drawn from the MAIN cache.
hexPacketTryCache:
    ld a,(hexPacketActive)
    or a
    ret z
    call hexCacheLookup
    ld a,0
    jp nz,hexPacketSetKind
    ld a,80h
hexPacketSetKind:
    ld (hexPacketKind),a
    call hexPacketPrepare
    ld a,(hexPacketKind)
    or a
    ret p
    ld hl,hexPacketRows
    inc (hl)
    scf
    ret

hexPacketPrepare:
    ld a,(hexPacketRows)
    or a
    jp z,hexPacketOpen
    ; Break before crossing the end of the direct-mapped slot array.
    ld hl,(hexDisplayRow)
    ld a,l
    ld hl,hexCacheMask
    and (hl)
    jp z,hexPacketNew
    ld hl,hexPacketGroup
    ld a,(hexPacketKind)
    cp (hl)
    jp nz,hexPacketNew
    or a
    ret m
    ld hl,hexLineLen
    ld a,(hexPacketBytes)
    add a,(hl)
    jp c,hexPacketNew
    cp EDIT_HEX_QUEUE_BYTES-GFX_RECORD_HEADER+1
    jp c,hexPacketTarget
hexPacketNew:
    call hexPacketFlush
hexPacketOpen:
    ld a,(hexPacketKind)
    ld (hexPacketGroup),a
    ld bc,EDIT_HEX_QUEUE_BYTES
    or a
    jp p,hexPacketReserve
    ld bc,GFX_RECORD_HEADER
hexPacketReserve:
    call commandQueueReserve
    jp c,ebFailed
    ld (hexPacketStart),hl
    IF EDIT_HEX_PSEUDO
    ld (hl),GFX_PSEUDO_ROWS
    ELSE
    ld (hl),GFX_TEXT_ROWS
    ENDIF
    inc hl
    ld (hl),COLOR_TEXT
    inc hl
    ld a,(hexY)
    ld (hl),a
    inc hl
    xor a
    ld b,3
hexPacketZero:
    ld (hl),a
    inc hl
    dec b
    jp nz,hexPacketZero
    ex de,hl
    ld hl,(hexCacheAddress)
    ex de,hl
    ld a,(hexPacketGroup)
    or a
    jp p,hexPacketInline
    ld (hl),e
    inc hl
    ld (hl),d
    inc hl
    ld de,0
    jp hexPacketDestination
hexPacketInline:
    ld (hl),0
    inc hl
    ld (hl),0
    inc hl
hexPacketDestination:
    ld (hl),e
    inc hl
    ld (hl),d
    inc hl
    ld a,(hexLineLen)
    ld (hl),a
    inc hl
    xor a
    ld (hl),a
    inc hl
    ld (hl),a
    inc hl
    ld a,(ebCacheStride)
    ld (hl),a
    inc hl
    inc hl
    inc hl
    ld (hexPacketEnd),hl
hexPacketTarget:
    ld a,(hexPacketGroup)
    or a
    ret m
    ld hl,(hexPacketEnd)
    ld (hexRowTarget),hl
    ret

hexPacketRow:
    ld hl,hexRow
    ld (hexRowTarget),hl
    ld a,(hexLineLen)
    ld c,a
    ld b,0
    ld hl,(hexPacketEnd)
    add hl,bc
    ld (hexPacketEnd),hl
    ld hl,hexPacketBytes
    ld a,(hl)
    add a,c
    ld (hl),a
    ld hl,hexPacketRows
    inc (hl)
    jp hexCachePublish

hexPacketFlush:
    ld a,(hexPacketRows)
    or a
    ret z
    ld hl,(hexPacketStart)
    ld de,11
    add hl,de
    ld (hl),a
    inc hl
    inc hl
    inc hl
    ld a,(hexPacketBytes)
    ld (hl),a
    inc hl
    xor a
    ld (hl),a
    ld (hexPacketRows),a
    ld (hexPacketBytes),a
    ld hl,(hexPacketEnd)
    jp commandQueueCommit

hexPacketActive: DB 0
hexPacketStart: DW 0
hexPacketEnd: DW 0
hexPacketRows: DB 0
hexPacketBytes: DB 0
hexPacketGroup: DB 0
hexPacketKind: DB 0
hexRowTarget: DW hexRow
