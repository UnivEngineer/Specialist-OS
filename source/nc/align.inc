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
    jp bios_printStringRawZ
 ; Same arguments, with B relative to the active panel.
ncPrintPanelAligned:
    push af
    ld a,(state.activePanel)
    or a
    jp z,ncPanelAlignedReady
    ld a,b
    add a,96
    ld b,a
ncPanelAlignedReady:
    pop af
    jp ncPrintAligned
ncPrintCentered:
    ld b,36
    ld c,120
    ld a,UI_ALIGN_CENTER
    jp ncPrintAligned

 ; HL=ASCIIZ, E=baseline. Center within the active panel's 8px side
; insets, clipping to one line. Source is unchanged; scratch is ncTextBuffer.
; All positioning is delegated to the common uiAlignText API above.
ncPrintPanelCentered:
    push de
    ld a,(ncFontWidth)
    rrca
    ld c,a
    ld a,88 ; Available half-pixels inside the panel.
    ld b,0
ncPanelCenterCapacity:
    sub c
    jp c,ncPanelCenterCopyStart
    inc b
    jp ncPanelCenterCapacity
ncPanelCenterCopyStart:
    ld de,ncTextBuffer
ncPanelCenterCopy:
    ld a,(hl)
    or a
    jp z,ncPanelCenterCopied
    ld (de),a
    inc hl
    inc de
    dec b
    jp nz,ncPanelCenterCopy
ncPanelCenterCopied:
    xor a
    ld (de),a
    pop de
    ld hl,ncTextBuffer
    ld b,4
    ld c,88
    ld a,UI_ALIGN_CENTER
    jp ncPrintPanelAligned

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
    jp bios_printStringRawZ

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
