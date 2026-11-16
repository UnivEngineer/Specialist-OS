; HL=text, B=left, C=field width (half-pixels), E=baseline, A=alignment.
; Text must fit; callers compact paths before using this entry.
ncPrintAligned:
    push af
    ld a,(ncFontWidth)
    ld d,a
    pop af
    call uiAlignText
    ret c
    push hl
    ex de,hl
    call bios_setCursorPos
    pop hl
    jp bios_printStringReal
ncPrintCentered:
    ld b,36
    ld c,120
    ld a,UI_ALIGN_CENTER
    jp ncPrintAligned

; Same contract as ncPrintField, right padding is moved before the text.
; The cursor is already at the left of the field. Text fits B characters.
ncPrintRightField:
    push bc
    push hl
    ld c,0
ncRightCount:
    ld a,(hl)
    or a
    jp z,ncRightCounted
    inc c
    inc hl
    jp ncRightCount
ncRightCounted:
    ld a,b
    sub c
    ld b,a
    ld de,ncTextBuffer
    jp z,ncRightText
ncRightSpaces:
    ld a,' '
    ld (de),a
    inc de
    dec b
    jp nz,ncRightSpaces
ncRightText:
    pop hl
ncRightCopy:
    ld a,(hl)
    ld (de),a
    inc hl
    inc de
    or a
    jp nz,ncRightCopy
    pop bc
    ld hl,ncTextBuffer
    jp bios_printStringReal

; Bind the current NC font; appearance and focus stay in the shared widget API.
ncDrawButton:
    push hl
    ld de,UI_BUTTON.font
    add hl,de
    ld a,(ncFontWidth)
    ld (hl),a
    inc hl
    inc hl
    ld (hl),COLOR_DIALOG
    inc hl
    ld a,COLOR_HELP_TEXT
    DUP 4
    rrca
    EDUP
    ld (hl),a
    pop hl
    jp uiDrawButton
