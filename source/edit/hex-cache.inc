; Cached glyphs use the same registered MAIN slots as text, with a private
; direct-mapped index. A tag is row/64, which fits in one byte even for a
; full EXEC buffer at eight bytes per screen row.
HEX_CACHE_ROWS = 128
hexCacheReset:
    ld a,(ebCacheRows)
    cp 128
    ld b,128
    jp nc,hexCacheSized
    cp 64
    ld b,64
    jp nc,hexCacheSized
    cp 32
    ld b,32
    jp nc,hexCacheSized
    ld b,0
hexCacheSized:
    ld a,b
    ld (hexCacheSlots),a
    dec a
    ld (hexCacheMask),a
    ld hl,hexCacheTags
    ld b,HEX_CACHE_ROWS
    ld a,0FFh
hexCacheClear:
    ld (hl),a
    inc hl
    dec b
    jp nz,hexCacheClear
    ld hl,0FFFFh
    ld (hexPrefetchTop),hl
    ret

; Return Z on hit. Regardless of hit, remember the destination and tag.
hexCacheLookup:
    ld hl,0
    ld (hexCacheAddress),hl
    ld a,(hexCacheSlots)
    cp 1
    ret c
    ld hl,(hexDisplayRow)
    ld a,l
    rlca
    rlca
    and 3
    ld c,a
    ld a,h
    add a,a
    add a,a
    or c
    ld (hexCacheValue),a
    ld a,l
    ld hl,hexCacheMask
    and (hl)
    push af
    ld e,a
    ld d,0
    ld hl,hexCacheTags
    add hl,de
    ld (hexCacheTag),hl
    pop af
    call ebGlyphOffset
    ld de,EDIT_MAIN_CACHE
    add hl,de
    ld (hexCacheAddress),hl
    ld hl,(hexCacheTag)
    ld a,(hexCacheValue)
    cp (hl)
    ret

hexCachePublish:
    ld hl,(hexCacheAddress)
    ld a,h
    or l
    ret z
    ld hl,(hexCacheTag)
    ld a,(hexCacheValue)
    ld (hl),a
    ret

; Invalidate just the affected physical row even when it is off-screen.
hexCacheDrop:
    push hl
    ld a,l
    ld hl,hexCacheMask
    and (hl)
    ld e,a
    ld d,0
    ld hl,hexCacheTags
    add hl,de
    ld (hl),0FFh
    ld hl,0FFFFh
    ld (hexPrefetchTop),hl
    pop hl
    ret

; Prepare one glyph row per idle slice, first ahead, then behind. Leave
; all visible rows in the cache window.
hexPrefetchRow:
    ld a,(hexCacheSlots)
    or a
    ret z
    ld hl,(hexTop)
    ex de,hl
    ld hl,(hexPrefetchTop)
    call compareHLDE
    jp z,hexPrefetchNext
    ex de,hl
    ld (hexPrefetchTop),hl
    xor a
    ld (hexPrefetchBack),a
    ld a,(hexBar.visible)
    ld e,a
    ld d,0
    add hl,de
    ld (hexPrefetchAt),hl
    ld a,(hexCacheSlots)
    ld b,a
    sub e
    ld c,a                      ; slots beyond the visible page
    ld a,e
    dec a
    ld d,a
    ld a,b
    and 80h
    ld a,d
    jp z,hexPrefetchLimit
    add a,a                    ; enough space for two pages of lookahead
hexPrefetchLimit:
    cp c
    jp c,hexPrefetchPlan
    ld a,c
hexPrefetchPlan:
    ld (hexPrefetchAhead),a
    ld (hexPrefetchLeft),a
hexPrefetchNext:
    ld a,(hexPrefetchLeft)
    or a
    jp z,hexPrefetchReverse
    dec a
    ld (hexPrefetchLeft),a
    ld hl,(hexPrefetchAt)
    ex de,hl
    ld hl,(hexBar.total)
    call compareHLDE
    jp c,hexPrefetchReverse
    jp z,hexPrefetchReverse
    ex de,hl
    push hl
    ld a,(hexPrefetchBack)
    or a
    jp nz,hexPrefetchPrev
    inc hl
    jp hexPrefetchNextIndex
hexPrefetchPrev:
    dec hl
hexPrefetchNextIndex:
    ld (hexPrefetchAt),hl
    pop hl
    call hexDecodeRow
    call hexCacheLookup
    ret z
    ld a,(hexSummary)
    or a
    jp nz,hexPrefetchBlock
    call hexSetCount
    or a
    jp z,hexPrefetchEOF
    call hexSelectLine
    call hexFormatData
    jp hexPrefetchCopy
hexPrefetchBlock:
    call hexFormatBlock
    jp hexPrefetchCopy
hexPrefetchEOF:
    call hexMakeEOF
hexPrefetchCopy:
    ld hl,(hexCacheAddress)
    ex de,hl
    ld hl,hexRow
    ld a,(hexLineLen)
    ld c,a
    ld b,0
    call ebToMain
    ret c
    jp hexCachePublish

hexPrefetchReverse:
    ld a,(hexPrefetchBack)
    or a
    ret nz
    inc a
    ld (hexPrefetchBack),a
    ld hl,(hexTop)
    ld a,h
    or l
    ret z
    dec hl
    ld (hexPrefetchAt),hl
    ld hl,hexBar.visible
    ld a,(hexCacheSlots)
    sub (hl)
    ld hl,hexPrefetchAhead
    sub (hl)
    ld (hexPrefetchLeft),a
    jp hexPrefetchNext

hexCacheAddress:    DW 0
hexCacheTag:        DW 0
hexCacheSlots:      DB 0
hexCacheMask:       DB 0
hexCacheValue:      DB 0
hexPrefetchTop:     DW 0
hexPrefetchAt:      DW 0
hexPrefetchLeft:    DB 0
hexPrefetchBack:    DB 0
hexPrefetchAhead:   DB 0
