; Render 22 independently positioned lines with no automatic wrap.
gridPhaseRun:
    ld a,(gridKind)
    add a,a
    ld e,a
    ld d,0
    ld hl,gridPhaseRoutes
    add hl,de
    ld e,(hl)
    inc hl
    ld d,(hl)
    ex de,hl
    ld (gridPhaseCall+1),hl
    ld a,(gridPhase)
    ld d,a
    ld e,7
    ld a,(gridPseudoOnly)
    or a
    jp z,gridPhaseYReady
    inc e                      ; the entire first 10-pixel cell is visible
gridPhaseYReady:
    ld hl,gridText
    ld b,GRID_PHASE_ROWS
gridPhaseRow:
    ex de,hl
    ld (mon2_vars.cursorY),hl
    ex de,hl
    push bc
    push de
    push hl
    ld bc,GRID_GLYPHS
gridPhaseCall:
    call 0
    pop hl
    ld de,GRID_STRIDE
    add hl,de
    pop de
    ld a,e
    add a,10
    ld e,a
    pop bc
    dec b
    jp nz,gridPhaseRow
    ret

gridPhaseChar:
    ld b,c
.next:
    ld c,(hl)
    call mon2_printCharOld
    inc hl
    dec b
    jp nz,.next
    ret
gridPhaseRaw:
    ld b,c
.next:
    ld a,(hl)
    call FONT_ENGINE_RAW
    inc hl
    dec b
    jp nz,.next
    ret
gridPhaseRoutes:
    DW gridPhaseChar,gridPhaseRaw,mon2_printStringOld
    DW FONT_ENGINE_RAWZ,FONT_ENGINE_LIM,FONT_ENGINE_PSEUDO

gridPseudoSetup:
    xor a
    ld (gridPhase),a
    call engineSelectAll
    ld a,1
    ld (engineRecords+5),a       ; only the installed module has the public pseudo ABI
    ld a,'0'
    ld (gridPhasePixel),a
    ret
gridFirstKind:
    ld a,(gridPseudoOnly)
    or a
    ret z
    ld a,4
    ret

; Добавить два вертикальных разделителя и среднюю горизонтальную линию.
; Both entry points receive these exact same bytes and coordinates.
gridPseudoCorpus:
    ld hl,gridText
    ld b,GRID_PHASE_ROWS
gridPseudoCorpusRow:
    ld a,b
    cp GRID_PHASE_ROWS/2
    jp nz,gridPseudoVertical
    push hl
    ld c,GRID_GLYPHS
gridPseudoHorizontal:
    ld (hl),080h
    inc hl
    dec c
    jp nz,gridPseudoHorizontal
    pop hl
    ld a,08Ah
    jp gridPseudoDividers
gridPseudoVertical:
    ld a,081h
gridPseudoDividers:
    ld de,8
    add hl,de
    ld (hl),a
    ld de,24
    add hl,de
    ld (hl),a
    ld de,GRID_STRIDE-32
    add hl,de
    dec b
    jp nz,gridPseudoCorpusRow
    ret
gridPhaseTitle: DB '22 x 42 glyphs, MAIN X='
gridPhasePixel: DB '0',' px',13,10,0
