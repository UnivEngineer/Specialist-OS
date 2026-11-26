; MAIN captures the rectangle in free document space during HEX dialogs.
; Keep the gap sentinel and the text Undo snapshot outside the rectangle.
hexSaveUnderlay:
    push hl
    push de
    push bc
    push af
    xor a
    ld (hexUnderlay),a
    ld a,(hexActive)
    or a
    jp z,hexUnderlayDone
    inc hl
    ld e,(hl)
    inc hl
    ld d,(hl)
    inc hl
    ld a,(hl)
    add a,6
    ld c,a
    inc hl
    ld a,(hl)
    add a,2
    ld b,a
    ex de,hl
    ld (hexUnderlayVRAM),hl
    push bc
    pop hl
    ld (hexUnderlaySize),hl
    ld e,c
    ld d,0
    ld hl,1
hexUnderlayCount:
    add hl,de
    dec b
    jp nz,hexUnderlayCount
    push hl
    call gapFree
    pop de
    call compareHLDE
    jp c,hexUnderlayDone
    ld hl,(v_gapLo)
    inc hl
    ex de,hl
    ld hl,(hexUnderlayVRAM)
    push hl
    ld hl,(hexUnderlaySize)
    ld b,h
    ld c,l
    pop hl
    call uiBitmapSave
    ld a,1
    ld (hexUnderlay),a
hexUnderlayDone:
    pop af
    pop bc
    pop de
    pop hl
    ret
hexRestoreUnderlay:
    ld a,(hexUnderlay)
    or a
    jp z,hexRefresh
    xor a
    ld (hexUnderlay),a
    COLOR COLOR_TEXT
    ld hl,(hexUnderlaySize)
    ld b,h
    ld c,l
    ld hl,(v_gapLo)
    inc hl
    ex de,hl
    ld hl,(hexUnderlayVRAM)
    jp uiBitmapRestore
hexUnderlay:        DB 0
hexUnderlayVRAM:    DW 0
hexUnderlaySize:    DW 0
    DEFINE UI_BITMAP_RAW_ONLY
    INCLUDE "../../sdk/asm/lib/ui-bitmap.inc"
