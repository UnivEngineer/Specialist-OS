; HL=UI_BUTTON in local memory; uses uiAlignText and pixel rectangle helpers.
; uiDrawButton: opaque draw, frame, centered label, selected palette.
; uiSelectButton: A=0/1, update palette only when the state changes.
; AF/BC/DE/HL destroyed. Non-reentrant. Descriptor selects both palettes;
; the normal palette is restored after drawing or changing the focus.
; Override UI_BUTTON_CURSOR/PRINT with local MXCALL shims for banked callers.
    IFNDEF UI_BUTTON_CURSOR
    DEFINE UI_BUTTON_CURSOR bios_setCursorPos
    ENDIF
    IFNDEF UI_BUTTON_PRINT
    DEFINE UI_BUTTON_PRINT bios_printStringReal
    ENDIF
    IFNDEF UI_BUTTON_COLOR
    DEFINE UI_BUTTON_COLOR uiButtonSetColor
    ENDIF
uiDrawButton:
    ld (uiButtonObject),hl
    push hl
    ld de,UI_BUTTON.normal
    add hl,de
    ld a,(hl)
    call UI_BUTTON_COLOR
    pop hl
    call uiButtonRect
    xor a
    call uiFillRectPixels
    ld hl,(uiButtonObject)
    call uiButtonRect
    ld a,0FFh
    call uiFrameRectPixels
    ld hl,(uiButtonObject)
    inc hl
    inc hl
    ld b,(hl)
    inc hl
    ld c,(hl)
    inc hl
    ld d,(hl)
    inc hl
    ld a,(hl)
    or a
    rra
    add a,d
    add a,4
    ld e,a
    inc hl
    ld d,(hl)
    ld hl,(uiButtonObject)
    ld a,(hl)
    inc hl
    ld h,(hl)
    ld l,a
    ld a,UI_ALIGN_CENTER
    call uiAlignText
    ret c
    push hl
    ex de,hl
    call UI_BUTTON_CURSOR
    pop hl
    call UI_BUTTON_PRINT
    jp uiButtonPaint
uiSelectButton:
    and 1
    ld (uiButtonObject),hl
    ld de,UI_BUTTON.selected
    add hl,de
    cp (hl)
    ret z
    ld (hl),a
uiButtonPaint:
    ld hl,(uiButtonObject)
    ld de,UI_BUTTON.selected
    add hl,de
    ld a,(hl)
    inc hl
    or a
    jp z,uiButtonPalette
    inc hl
uiButtonPalette:
    ld a,(hl)
    call UI_BUTTON_COLOR
    ; Buttons are byte aligned. Rewrite unchanged bits to update hardware
    ; attributes over the whole rectangle, without a flash or color reads.
    ld hl,(uiButtonObject)
    call uiButtonRect
    IFDEF UI_BUTTON_RECOLOR
    call UI_BUTTON_RECOLOR
    ELSE
    ld a,l
    DUP 3
    rrca
    EDUP
    and 31
    ; X can be 256..383 too (bit 8 contributes screen column 32).
    ld l,a
    ld a,h
    or a
    jp z,uiButtonColumnReady
    ld a,l
    add a,32
    ld l,a
uiButtonColumnReady:
    ld a,l
    add a,090h
    ld h,a
    ld l,d
    ld a,c
    DUP 3
    rrca
    EDUP
    and 31
    ld b,a
    ld a,e
    ld c,a
uiButtonColorColumn:
    push bc
    push hl
uiButtonColorRow:
    ld a,(hl)
    ld (hl),a
    inc l
    dec c
    jp nz,uiButtonColorRow
    pop hl
    pop bc
    inc h
    dec b
    jp nz,uiButtonColorColumn
    ENDIF
    ld hl,(uiButtonObject)
    ld de,UI_BUTTON.normal
    add hl,de
    ld a,(hl)
    jp UI_BUTTON_COLOR
uiButtonSetColor:
    ld (IO_COLOR),a
    ret
uiButtonRect:
    push hl
    ld de,UI_BUTTON.width
    add hl,de
    ld a,(hl)
    add a,a
    ld c,a
    ld a,0
    adc a,0
    ld b,a
    inc hl
    ld d,(hl)
    inc hl
    ld e,(hl)
    pop hl
    inc hl
    inc hl
    ld l,(hl)
    ld h,0
    add hl,hl
    ret
uiButtonObject: DW 0
