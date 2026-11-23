; Shared modal rendering; include after ui.inc implementation and ui-button.inc.
; HL=UI_MODAL_BEGIN descriptor. A=current glyph width. AF/BC/DE/HL destroyed.
; uiModalOpen snapshots via client hook, uiModalClear repaints the same window
; and its buttons, including all separators. The caller supplies content and
; actions. No client-supplied frame/button coordinates or clear sizes.
    IFNDEF UI_MODAL_SNAPSHOT
    DEFINE UI_MODAL_SNAPSHOT uiModalNoSnapshot
    ENDIF
    IFNDEF UI_MODAL_BUTTON_DRAW
    DEFINE UI_MODAL_BUTTON_DRAW uiDrawButton
    ENDIF
uiModalOpen:
    ld (uiModalFont),a
    xor a
    ld (uiModalCount),a
    jp uiModalOpenReady
; Same as Open, B=number of populated cells (for a partial last grid row).
uiModalOpenCount:
    ld (uiModalFont),a
    ld a,b
    ld (uiModalCount),a
uiModalOpenReady:
    ld (uiModalCurrent),hl
    ld de,7
    add hl,de
    call UI_MODAL_SNAPSHOT
    ld hl,(uiModalCurrent)
    jp uiModalClear
; Compact preset: C=panel (0/1), B=populated count, A=font, HL=descriptor.
; Relocation is owned here; reopening on the other panel applies a delta.
uiModalPanelOpen:
    push af
    push bc
    push hl
    ld de,9
    add hl,de
    ld a,c
    or a
    ld a,093h
    jp z,uiModalPanelTarget
    ld a,0ABh
uiModalPanelTarget:
    sub (hl)
    ld (uiModalDelta),a
    add a,(hl)
    ld (hl),a
    ld de,5
    add hl,de
    ld a,(uiModalDelta)
    add a,(hl)
    ld (hl),a
    ld de,4
    add hl,de
    ld a,(hl)
    or a
    jp z,uiModalPanelButtons
    inc hl
    inc hl
    ld a,(uiModalDelta)
    add a,(hl)
    ld (hl),a
uiModalPanelButtons:
    pop hl
    push hl
    xor a
    push bc
    call uiModalButtonAt
    pop bc
    ld a,(uiModalDelta)
    add a,a
    add a,a
    ld c,a
uiModalPanelButton:
    inc hl
    inc hl
    ld a,(hl)
    add a,c
    ld (hl),a
    ld de,UI_BUTTON-2
    add hl,de
    dec b
    jp nz,uiModalPanelButton
    pop hl
    pop bc
    pop af
    jp uiModalOpenCount
uiModalNoSnapshot:
    ret
uiModalClear:
    ld (uiModalCurrent),hl
    ld de,7
    add hl,de
    xor a
    call uiDraw
    ld hl,(uiModalCurrent)
    ld de,4
    add hl,de
    ld b,(hl)
    ld a,b
    or a
    ret z
    ld a,(uiModalCount)
    or a
    jp z,uiModalCountReady
    ld b,a
uiModalCountReady:
    inc hl
    ld e,(hl)
    inc hl
    ld d,(hl)
    ex de,hl
uiModalButtons:
    push bc
    push hl
    ld de,UI_BUTTON.font
    add hl,de
    ld a,(uiModalFont)
    ld (hl),a
    pop hl
    push hl
    call UI_MODAL_BUTTON_DRAW
    pop hl
    ld de,UI_BUTTON
    add hl,de
    pop bc
    dec b
    jp nz,uiModalButtons
    ret
; HL=text. Centered title/body line, using the same body padding as widgets.
uiModalTitle:
    push hl
    ld hl,(uiModalCurrent)
    ld e,(hl)
    jp uiModalText
uiModalBodyText:
    push hl
    ld hl,(uiModalCurrent)
    inc hl
    ld e,(hl)
    inc hl
    ld a,(hl)
    or a
    rra
    add a,e
    add a,3
    ld e,a
uiModalText:
    pop hl
    push hl
    ld hl,(uiModalCurrent)
    ld bc,9
    add hl,bc
    ld a,(hl)
    sub 90h
    add a,a
    add a,a
    add a,4
    ld b,a
    inc hl
    inc hl
    ld a,(hl)
    add a,a
    add a,a
    ld c,a
    pop hl
    ld a,(uiModalFont)
    ld d,a
    ld a,UI_ALIGN_CENTER
    call uiAlignText
    ret c
    push hl
    ex de,hl
    call bios_setCursorPos
    pop hl
    jp bios_printStringReal
; Body-relative baseline: A=offset, returns E=absolute baseline. HL preserved.

;----------------------------------------------------------------------------
; MXOS
;
; Перевести вертикальное смещение элемента в координату модального окна.
;
; вход:
;   a = смещение, uiModalCurrent содержит адрес описателя с координатой Y во втором
;   байте.
;
; выход:
;   e и a = базовая координата плюс смещение; cf = перенос. hl, bc и d сохранены.
;----------------------------------------------------------------------------

uiModalBodyY:
    push hl
    ld hl,(uiModalCurrent)
    inc hl
    add a,(hl)
    ld e,a
    pop hl
    ret
; HL=descriptor, A=zero-based populated cell. Return HL=UI_BUTTON;
; BC/DE/AF destroyed. Use for keyboard focus or dynamic captions.
uiModalButtonAt:
    push af
    ld de,5
    add hl,de
    ld e,(hl)
    inc hl
    ld d,(hl)
    pop af
    ld l,a
    ld h,0
    add hl,hl
    ld b,h
    ld c,l
    add hl,hl
    add hl,hl
    add hl,bc
    add hl,de
    ret
    IFDEF UI_MODAL_KEYS
; Button focus is drawn by the modal. Only lineEdit has a text caret.
; Same key in A and modifier flags as bios_getch, including CF.
uiModalGetch:
    push bc
    ld a,(bios_upper.cursorCfg)
    push af
    xor a
    ld (bios_upper.cursorCfg),a
    call bios_getch
    push af
    pop bc
    pop af
    ld (bios_upper.cursorCfg),a
    push bc
    pop af
    pop bc
    ret
    INCLUDE "ui-modal-focus.inc"
    ENDIF
uiModalCurrent:     DW 0
uiModalFont:        DB 6
uiModalCount:       DB 0
uiModalDelta:       DB 0
