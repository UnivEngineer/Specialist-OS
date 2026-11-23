; Drive lists use linear navigation even when broken across two rows.
UI_FOCUS_GRID   = 0
UI_FOCUS_LINEAR = 1
UI_FOCUS_PAIR   = 2  ; Left selects the first button, Right selects the second.

; Configure before drawing. HL=descriptor, A=initial index, B=mode,
; C=populated count (0=whole descriptor). AF/BC/DE/HL destroyed.
uiModalFocusInit:
    ld (uiModalFocusIndex),a
    ld a,b
    ld (uiModalFocusMode),a
    ld de,4
    add hl,de
    ld a,c
    or a
    jp nz,uiFocusCountReady
    ld a,(hl)
uiFocusCountReady:
    ld (uiFocusCount),a
    ld b,a
    ld a,(uiModalFocusIndex)
    cp b
    jp c,uiFocusInitIndexReady
    xor a
    ld (uiModalFocusIndex),a
uiFocusInitIndexReady:
    ld c,0
    xor a
    ld (uiFocusCols),a
    ld a,b
    or a
    ret z
    inc hl
    ld e,(hl)
    inc hl
    ld d,(hl)
    ex de,hl
uiFocusInitButton:
    push hl
    ld de,UI_BUTTON.top
    add hl,de
    ld a,c
    or a
    ld a,(hl)
    jp nz,uiFocusCompareTop
    ld (uiFocusTop),a
uiFocusCompareTop:
    ld e,a
    ld a,(uiFocusTop)
    cp e
    jp nz,uiFocusInitSelected
    ld a,(uiFocusCols)
    inc a
    ld (uiFocusCols),a
uiFocusInitSelected:
    ld de,UI_BUTTON.selected-UI_BUTTON.top
    add hl,de
    ld a,(uiModalFocusIndex)
    cp c
    ld a,0
    jp nz,uiFocusInitStore
    inc a
uiFocusInitStore:
    ld (hl),a
    pop hl
    ld de,UI_BUTTON
    add hl,de
    inc c
    dec b
    jp nz,uiFocusInitButton
    ret

; Wait for a selection or Escape. A=index, CF=0 on Enter, CF=1 on Escape.
uiModalChoose:
    call uiModalReadChoice
    ret nc
    cp 1Bh
    jp nz,uiModalChoose
    scf
    ret

; Enter returns A=index, CF=0. Escape and client hotkeys return A=key, CF=1.
uiModalReadChoice:
    call uiModalGetch
    call uiModalFocusKey
    jp c,uiModalReadChoice
    cp 13
    jp nz,uiFocusUnhandled
    ld a,(uiModalFocusIndex)
    or a
    ret
uiFocusUnhandled:
    scf
    ret

; Dispatch a single key for a nonblocking client. CF marks consumed navigation.
; Other keys stay in A with CF=0.
uiModalFocusKey:
    ld e,a
    ld a,(uiFocusCount)
    ld b,a
    cp 2
    jp c,uiFocusPass
    ld a,(uiModalFocusIndex)
    ld c,a
    ld a,e
    cp 9
    jp z,uiFocusNext
    cp 8
    jp z,uiFocusLeft
    cp 18h
    jp z,uiFocusRight
    cp 19h
    jp z,uiFocusUp
    cp 1Ah
    jp z,uiFocusDown
uiFocusPass:
    ld a,e
    or a
    ret
uiFocusLeft:
    ld a,(uiModalFocusMode)
    cp UI_FOCUS_PAIR
    jp z,uiFocusFirst
    or a
    jp nz,uiFocusPrevious
    call uiFocusColumn
    or a
    jp z,uiFocusRowLast
    jp uiFocusPrevious
uiFocusRight:
    ld a,(uiModalFocusMode)
    cp UI_FOCUS_PAIR
    jp z,uiFocusLast
    or a
    jp nz,uiFocusNext
    call uiFocusColumn
    inc a
    cp d
    jp z,uiFocusRowFirst
    ld a,c
    inc a
    cp b
    jp z,uiFocusRowFirst
    jp uiFocusSelect
uiFocusColumn:
    ld a,(uiFocusCols)
    ld d,a
    ld a,c
uiFocusColumnLoop:
    cp d
    ret c
    sub d
    jp uiFocusColumnLoop
uiFocusRowFirst:
    call uiFocusColumn
    ld e,a
    ld a,c
    sub e
    jp uiFocusSelect
uiFocusRowLast:
    ld a,c
    add a,d
    cp b
    jp nc,uiFocusLast
    dec a
    jp uiFocusSelect
uiFocusUp:
    ld a,(uiModalFocusMode)
    or a
    jp nz,uiFocusPrevious
    ld a,(uiFocusCols)
    cp b
    jp z,uiFocusPrevious
    ld d,a
    ld a,c
    sub d
    jp nc,uiFocusSelect
    ld a,c
    ; Wrap to the last row that has this column.
uiFocusBottom:
    add a,d
    cp b
    jp c,uiFocusBottom
    sub d
    jp uiFocusSelect
uiFocusDown:
    ld a,(uiModalFocusMode)
    or a
    jp nz,uiFocusNext
    ld a,(uiFocusCols)
    cp b
    jp z,uiFocusNext
    add a,c
    cp b
    jp nc,uiFocusRowTop
    jp uiFocusSelect
uiFocusRowTop:
    call uiFocusColumn
    jp uiFocusSelect
uiFocusPrevious:
    ld a,c
    or a
    jp z,uiFocusLast
    dec a
    jp uiFocusSelect
uiFocusNext:
    ld a,c
    inc a
    cp b
    jp c,uiFocusSelect
uiFocusFirst:
    xor a
    jp uiFocusSelect
uiFocusLast:
    ld a,b
    dec a
uiFocusSelect:
    cp c
    jp z,uiFocusConsumed
    push af
    ld a,c
    call uiFocusButtonAt
    xor a
    call uiSelectButton
    pop af
    ld (uiModalFocusIndex),a
    call uiFocusButtonAt
    ld a,1
    call uiSelectButton
uiFocusConsumed:
    scf
    ret
uiFocusButtonAt:
    ld hl,(uiModalCurrent)
    jp uiModalButtonAt

uiModalFocusIndex:  DB 0
uiModalFocusMode:   DB 0
uiFocusCount:       DB 0
uiFocusCols:        DB 0
uiFocusTop:         DB 0
