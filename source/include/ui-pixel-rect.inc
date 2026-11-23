; Pixel rectangles for the 384x256 screen (opt in with UI_PIXEL_RECT).
; HL=X (0..383), D=Y, BC=width (0..384), E=height (0..255).
; Rectangle must be inside the screen; zero width/height is a no-op.
; uiFillRectPixels: A=byte pattern (00h clear, FFh solid); aligned to VRAM bits.
; uiInverseRectPixels: XOR all rectangle bits; A ignored. Calling twice restores.
; uiFrameRectPixels: same arguments as fill; one-pixel outline, interior intact.
; Edge masks preserve all outside bitmap pixels. The caller sets IO_COLOR;
; hardware attributes belong to whole bytes, including partially masked edges.
; AF/BC/DE/HL destroyed; normal SP use, no interrupt requirement. Non-reentrant.
; HL=G_WINDOW/G_MODAL_WINDOW descriptor, A=relative X offset in screen bytes
; (as uiDraw; ignored for absolute descriptors). Clear content, including
; separators/buttons, preserving both border lines and modal outer padding.
; Color is set by caller; AF/BC/DE/HL destroyed. Requires a valid window.
    INCLUDE "ui-clear-window.inc"
; Input background: HL=X, D=glyph baseline, BC=field width, A=glyph height.
; Solid inverse field with one extra pixel above and below the glyph rows.
; Text/cursor baseline stays unchanged. Color is set by caller; all destroyed.
uiInputFieldPixels:
    ld e,a
    ld a,d
    sub e
    ld d,a
    inc e
    inc e
    ld a,0FFh
    jp uiFillRectPixels
uiFillRectPixels:
    ld (uiRectPattern),a
    xor a
    ld (uiRectInverse),a
    jp uiRectBegin
uiInverseRectPixels:
    ld a,1
    ld (uiRectInverse),a
uiRectBegin:
    ld a,b
    or c
    ret z
    ld a,e
    or a
    ret z
    ld (uiRectHeight),a
    ld a,d
    ld (uiRectY),a
    push hl
    add hl,bc
    dec hl
    ld a,l
    and 7
    inc a
    call uiRectMask
    ld (uiRectLastMask),a
    call uiRectByteAddress
    ld (uiRectLastColumn),a
    pop hl
    ld a,l
    and 7
    call uiRectMask
    cpl
    ld (uiRectFirstMask),a
    call uiRectByteAddress
    ld h,a
    ld a,(uiRectY)
    ld l,a
    ld a,(uiRectLastColumn)
    sub h
    inc a
    ld (uiRectColumns),a
    ld a,(uiRectFirstMask)
    ld d,a
uiRectColumn:
    ld a,(uiRectColumns)
    cp 1
    jp nz,uiRectMaskReady
    ld a,(uiRectLastMask)
    and d
    ld d,a
uiRectMaskReady:
    push hl
    ld a,(uiRectHeight)
    ld c,a
    ld a,(uiRectInverse)
    or a
    jp nz,uiRectInverseRows
    ld a,(uiRectPattern)
    and d
    ld e,a
    ld a,d
    cpl
    ld b,a
uiRectFillRows:
    ld a,(hl)
    and b
    or e
    ld (hl),a
    inc l
    dec c
    jp nz,uiRectFillRows
    jp uiRectNextColumn
uiRectInverseRows:
    ld a,(hl)
    xor d
    ld (hl),a
    inc l
    dec c
    jp nz,uiRectInverseRows
uiRectNextColumn:
    pop hl
    inc h
    ld d,0FFh
    ld a,(uiRectColumns)
    dec a
    ld (uiRectColumns),a
    jp nz,uiRectColumn
    ret
; A=number of high bits (0..8), result A; HL/BC preserved, DE destroyed.
uiRectMask:
    push hl
    ld hl,uiRectMasks
    ld e,a
    ld d,0
    add hl,de
    ld a,(hl)
    pop hl
    ret
; HL=pixel X -> A=VRAM column (90h..BFh); HL/BC preserved, E destroyed.
uiRectByteAddress:
    ld a,l
    rrca
    rrca
    rrca
    and 01Fh
    ld e,a
    ld a,h
    rrca
    rrca
    rrca
    and 020h
    or e
    add a,090h
    ret
uiRectMasks:        DB 0,080h,0C0h,0E0h,0F0h,0F8h,0FCh,0FEh,0FFh
uiRectPattern:      DB 0
uiRectInverse:      DB 0
uiRectHeight:       DB 0
uiRectY:            DB 0
uiRectFirstMask:    DB 0
uiRectLastMask:     DB 0
uiRectLastColumn:   DB 0
uiRectColumns:      DB 0

uiFrameRectPixels:
    ld (uiFramePattern),a
    ld a,b
    or c
    ret z
    ld a,e
    or a
    ret z
    ld (uiFrameHeight),a
    ld a,d
    ld (uiFrameY),a
    ld (uiFrameX),hl
    ld h,b
    ld l,c
    ld (uiFrameWidth),hl
    ld hl,(uiFrameX)
    ld e,1
    ld a,(uiFramePattern)
    call uiFillRectPixels
    ld a,(uiFrameHeight)
    cp 1
    ret z
    dec a
    ld b,a
    ld a,(uiFrameY)
    add a,b
    ld d,a
    ld e,1
    ld hl,(uiFrameWidth)
    ld b,h
    ld c,l
    ld hl,(uiFrameX)
    ld a,(uiFramePattern)
    call uiFillRectPixels
    ld a,(uiFrameHeight)
    cp 3
    ret c
    ld hl,(uiFrameX)
    call uiFrameSide
    ld hl,(uiFrameWidth)
    ld a,h
    or a
    jp nz,uiFrameRight
    ld a,l
    cp 1
    ret z
uiFrameRight:
    dec hl
    ex de,hl
    ld hl,(uiFrameX)
    add hl,de
uiFrameSide:
    ld a,(uiFrameHeight)
    sub 2
    ld e,a
    ld a,(uiFrameY)
    inc a
    ld d,a
    ld bc,1
    ld a,(uiFramePattern)
    jp uiFillRectPixels
uiFrameX:           DW 0
uiFrameWidth:       DW 0
uiFrameY:           DB 0
uiFrameHeight:      DB 0
uiFramePattern:     DB 0

; Font-independent 8x8 checkbox bitmap. HL=byte-aligned VRAM address,
; A=0 unchecked / nonzero checked. Caller sets color. AF/BC/DE destroyed,
; HL advances eight rows. Whole-byte writes set the same hardware palette.
uiDrawCheckbox:
    or a
    ld de,uiCheckboxEmpty
    jp z,uiCheckboxReady
    ld de,uiCheckboxTick
uiCheckboxReady:
    ld b,8
uiCheckboxRow:
    ld a,(de)
    ld (hl),a
    inc de
    inc l
    dec b
    jp nz,uiCheckboxRow
    ret
uiCheckboxEmpty:    DB 0FFh,081h,081h,081h,081h,081h,081h,0FFh
uiCheckboxTick:     DB 0FFh,083h,087h,08Dh,0D9h,0B1h,0A1h,0FFh
