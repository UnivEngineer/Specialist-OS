; HL = UI_SCROLLBAR. Uses UI_PIXEL_RECT. Caller chooses IO_COLOR.
; Draw a stippled track and a solid thumb; counts are unsigned 16-bit units.
; The position is clamped and both ends of the track are reachable.
; AF/BC/DE/HL destroyed. Does not change the text cursor or inverse state.
uiDrawScrollbar:
    ld (scrollDescriptor),hl
    ld de,UI_SCROLLBAR.length
    add hl,de
    ld a,(hl)
    or a
    ret z
    ld (scrollLength),a
    inc hl
    ld a,(hl)
    or a
    ret z
    ld (scrollThickness),a
    inc hl
    ld a,(hl)
    ld (scrollHorizontal),a
    inc hl
    ld e,(hl)
    inc hl
    ld d,(hl)
    inc hl
    ld c,(hl)
    inc hl
    ld b,(hl)
    inc hl
    ld a,(hl)
    inc hl
    ld h,(hl)
    ld l,a
    ld (scrollPosition),hl
    ld h,d
    ld l,e
    ld a,l
    sub c
    ld l,a
    ld a,h
    sbc a,b
    ld h,a
    jp c,scrollFullThumb
    or l
    jp z,scrollFullThumb
    ld (scrollRange),hl
    ld a,(scrollLength)
    call scrollScale
    ld b,a
    ; Eight pixels along the track, in the descriptor's length units.
    ld a,(scrollHorizontal)
    cp 2
    ld a,8
    jp nz,scrollThumbMinimum
    ld a,1
scrollThumbMinimum:
    cp b
    jp c,scrollThumbProportional
    ld b,a
    ld a,(scrollLength)
    cp b
    jp c,scrollThumbReady
scrollThumbProportional:
    ld a,b
scrollThumbReady:
    ld (scrollThumb),a
    ld b,a
    ld a,(scrollLength)
    sub b
    push af
    ld hl,(scrollRange)
    ex de,hl
    ld hl,(scrollPosition)
    ld a,h
    cp d
    jp nz,scrollPositionCompared
    ld a,l
    cp e
scrollPositionCompared:
    jp c,scrollPositionReady
    ex de,hl
    ld d,h
    ld e,l
scrollPositionReady:
    ld b,h
    ld c,l
    pop af
    call scrollScale
    jp scrollDraw
scrollFullThumb:
    ld a,(scrollLength)
    ld (scrollThumb),a
    xor a
scrollDraw:
    push af
    ld hl,(scrollDescriptor)
    ld a,(hl)
    inc hl
    ld b,(hl)
    inc hl
    ld d,(hl)
    ld h,b
    ld l,a
    push hl
    push de
    ld a,(scrollLength)
    ld e,a
    ld a,(scrollHorizontal)
    cp 2
    ld a,0
    jp z,scrollTrackPattern
    ld a,(scrollHorizontal)
    cp 3
    ld a,08h
    jp z,scrollTrackPattern
    ld a,055h
scrollTrackPattern:
    call scrollRect
    pop de
    pop hl
    ld a,(scrollHorizontal)
    cp 2
    call z,scrollHorizontalLine
    pop af
    ld e,a
    ld a,(scrollHorizontal)
    or a
    jp z,scrollOffsetY
    cp 3
    jp z,scrollOffsetY
    push de
    ld d,0
    dec a
    jp z,scrollByteOffsetReady
    ex de,hl
    add hl,hl
    add hl,hl
    add hl,hl
    ex de,hl
scrollByteOffsetReady:
    add hl,de
    pop de
    jp scrollDrawThumb
scrollOffsetY:
    ld a,d
    add a,e
    ld d,a
scrollDrawThumb:
    ld a,(scrollThumb)
    ld e,a
    ld a,(scrollHorizontal)
    cp 3
    ld a,07Fh
    jp z,scrollRect
    ld a,0FFh
scrollRect:
    push af
    ld a,(scrollHorizontal)
    or a
    jp z,scrollRectVertical
    cp 3
    jp z,scrollRectVertical
    ld c,e
    ld b,0
    dec a
    jp z,scrollRectThickness
    push hl
    ld h,b
    ld l,c
    add hl,hl
    add hl,hl
    add hl,hl
    ld b,h
    ld c,l
    pop hl
scrollRectThickness:
    ld a,(scrollThickness)
    ld e,a
    jp scrollRectReady
scrollRectVertical:
    ld a,(scrollThickness)
    ld c,a
    ld b,0
scrollRectReady:
    pop af
    jp uiFillRectPixels

scrollHorizontalLine:
    push hl
    push de
    ld a,1
    ld (scrollThickness),a
    ld a,d
    add a,4
    ld d,a
    ld a,(scrollLength)
    ld e,a
    ld a,0FFh
    call scrollRect
    ld a,7
    ld (scrollThickness),a
    pop de
    pop hl
    inc d
    ret

; Floor(A * BC / DE) for BC <= DE and DE != 0. Accumulate the remainder
; against DE-BC before adding BC, so the 16-bit sum cannot overflow.
scrollScale:
    or a
    ret z
    push af
    ld a,e
    sub c
    ld e,a
    ld a,d
    sbc a,b
    ld d,a
    ld hl,0
    xor a
    ld (scrollQuotient),a
    pop af
scrollScaleStep:
    push af
    ld a,l
    sub e
    ld a,h
    sbc a,d
    jp c,scrollScaleAdd
    ld h,a
    ld a,l
    sub e
    ld l,a
    ld a,(scrollQuotient)
    inc a
    ld (scrollQuotient),a
    jp scrollScaleNext
scrollScaleAdd:
    add hl,bc
scrollScaleNext:
    pop af
    dec a
    jp nz,scrollScaleStep
    ld a,(scrollQuotient)
    ret
scrollDescriptor:   DW 0
scrollPosition:     DW 0
scrollRange:        DW 0
scrollLength:       DB 0
scrollThickness:    DB 0
scrollHorizontal:   DB 0
scrollThumb:        DB 0
scrollQuotient:     DB 0
