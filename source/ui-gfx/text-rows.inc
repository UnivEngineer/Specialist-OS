; Render a fixed-pitch group of glyph rows. Inline rows may also populate a
; caller's MAIN cache. A cached source already has the slot stride.
ngPseudoRows:
    push hl
    ld hl,bios_printStringRawPseudo
    jp ngRowsSelectPrint
ngTextRows:
    push hl
    ld hl,bios_printStringRawLim
ngRowsSelectPrint:
    ld (ngRowsPrintCall+1),hl
    pop hl
    ld (ngRowsSource),hl
    ld (ngRowsStride),a
    ex de,hl
    ld (ngRowsDestination),hl
    ld a,b
    or a
    jp z,ngRowsBad
    ld (ngRowsLeft),a
    ld a,c
    or a
    jp z,ngRowsBad
    ld (ngRowsWidth),a
    ld (ngRowsStep),a
    ld hl,(bios_vars.cursorY)
    ld (ngRowsCursor),hl
    ld hl,(ngPayload)
    push hl
    ex de,hl
    ld hl,(ngRowsSource)
    ex de,hl
    ld a,d
    or e
    jp nz,ngRowsLength
    ld e,c
    ld d,0
ngRowsLengthLoop:
    add hl,de
    dec b
    jp nz,ngRowsLengthLoop
ngRowsLength:
    ex de,hl
    ld hl,(ngNextRecord)
    call bios_cmp_hl_de
    pop hl
    jp nz,ngRowsBad
    ex de,hl
    ld hl,(ngRowsSource)
    ex de,hl
    ld a,d
    or e
    jp z,ngRowsNext
    ex de,hl
    ld a,(ngRowsStride)
    ld (ngRowsStep),a
ngRowsNext:
    push hl
    ex de,hl
    ld hl,(ngRowsDestination)
    ex de,hl
    ld a,d
    or e
    jp z,ngRowsPrint
    ld a,(ngRowsWidth)
    ld c,a
    ld b,0
    add hl,bc
    ld b,d
    ld c,e
    ex de,hl
    pop hl
    push hl
    call bios_memcpy_bc_hl
    ld hl,(ngRowsDestination)
    ld a,(ngRowsStride)
    ld e,a
    ld d,0
    add hl,de
    ld (ngRowsDestination),hl
ngRowsPrint:
    ld hl,(ngRowsCursor)
    ld (bios_vars.cursorY),hl
    ld a,l
    add a,10
    ld l,a
    ld (ngRowsCursor),hl
    pop hl
    push hl
    ld a,(ngRowsWidth)
    ld c,a
    ld b,0
ngRowsPrintCall:
    call 0
    pop hl
    ld a,(ngRowsLeft)
    dec a
    ld (ngRowsLeft),a
    ret z
    ld a,(ngRowsStep)
    ld e,a
    ld d,0
    add hl,de
    jp ngRowsNext
ngRowsBad:
    pop hl                     ; discard the dispatcher continuation
    jp ngBad

ngRowsCursor:       DW 0
ngRowsSource:       DW 0
ngRowsDestination:  DW 0
ngRowsLeft:         DB 0
ngRowsWidth:        DB 0
ngRowsStride:       DB 0
ngRowsStep:         DB 0
