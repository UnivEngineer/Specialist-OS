; Entry from tryGroup after validating X and ESC state. A is X mod 4.
; Plan one run before drawing: printable text, ASCIZ, or a bounded array.
    IFDEF FONT_ALIGNED_ROWS
graphicAlignedPacket:
    ELSE
graphicPacket:
    dec a
    ld (graphPhase),a
    ENDIF
    ld a,(mon2_vars.cursorX)
    rrca
    rrca
    and 3Fh
    ld d,a
    add a,high MX_SCREEN_BASE
    ld (graphStart),a
    ld a,48
    sub d
    ld e,a
    ld a,(fast6.realMode)
    cp 2
    jp nc,graphLimited
    or a
    ld b,1
    jp nz,graphScanStart
    ld b,20h
graphScanStart:
    ld c,0
graphScan:
    ld a,(hl)
    cp b
    jp c,graphPlanned
    inc hl
    inc c
    dec e
    jp nz,graphScan
    jp graphPlanned
graphLimited:
    ld a,b
    or a
    jp nz,graphUseQuota
    ld a,c
    cp e
    jp c,graphCountReady
graphUseQuota:
    ld c,e
graphCountReady:
    ld a,(fast6.realMode)
    add a,a
    jp nz,graphRawCountReady
    ld e,c
    ld b,20h
    jp graphScanStart
graphRawCountReady:
    ld b,0
    add hl,bc
graphPlanned:
    ld a,c
    cp 2
    jp c,fast6.empty       ; let the scalar entry handle isolated bytes
    ld (graphNext),hl
    ld a,(graphStart)
    add a,c
    ld (graphEnd),a
    IFDEF FONT_ALIGNED_ROWS
    INCLUDE "aligned8-rows.inc"
    ELSE
    IFDEF FONT_GRAPHIC_PAIR
    IFDEF FONT_GRAPHIC_QUAD
    ld a,c
    and 0FCh
    ld b,a
    ld a,(graphStart)
    add a,b
    ld (graphQuadEnd),a
    ENDIF
    ld a,c
    and 1
    ld b,a
    ld a,(graphEnd)
    sub b
    ld (graphPairEnd),a
    dec hl
    ld a,(hl)
    ld (graphLast),a
    ld hl,0
    add hl,sp
    ld (graphSavedSP+1),hl
    ENDIF
    ld a,(graphPhase)
    ld l,a
    ld a,(mon2_vars.inverse)
    or a
    ld a,l
    jp z,graphSelect
    add a,3
graphSelect:
    add a,a
    ld e,a
    ld d,0
    ld hl,graphRoutes
    add hl,de
    ld e,(hl)
    inc hl
    ld d,(hl)
    ex de,hl
    ld (graphJump+1),hl
    ld a,(mon2_vars.cursorY)
    sub 7
    ld e,a
    IF GRAPH_PRE_MODE
    ld a,(graphPhase)
    call graphPrePage
    ELSE
    ld h,FONT_ADDR/256
    ENDIF
graphJump:
    jp 0

    MACRO GRAPH_ROTATE shift,inverse
    IF inverse
    cpl
    ENDIF
    IF !GRAPH_PRE_MODE
    IF shift == 6
    rlca
    rlca
    ELSE
    DUP shift
    rrca
    EDUP
    ENDIF
    ENDIF
    ENDM

    MACRO GRAPH_FULL_GLYPH inverse
    or (hl)
    IF inverse
    cpl
    ENDIF
    ld (de),a
    inc d
    inc h
    ld a,(hl)
    dec h
    ENDM

; B holds the second code, C the rotated first glyph. Leave the second
; glyph's spill in A so a four-byte iteration can feed the next pair.
    MACRO GRAPH_SECOND shift,inverse
    ld l,b
    ld a,(hl)
    GRAPH_ROTATE shift,inverse
    ld b,a
    xor c
    and 255>>shift
    xor c
    ld (de),a
    inc d
    ld a,b
    and 256-(256>>shift)
    ENDM

    MACRO GRAPH_PAIR_FROM_A shift,inverse
    pop bc
    ld l,c
    IF GRAPH_PRE_MODE == 2
    GRAPH_FULL_GLYPH inverse
    ld l,b
    GRAPH_FULL_GLYPH inverse
    ELSE
    ld c,a
    ld a,(hl)
    GRAPH_ROTATE shift,inverse
    ld l,a
    and 255>>shift
    or c
    ld (de),a
    inc d
    ld c,l
    GRAPH_SECOND shift,inverse
    ENDIF
    ENDM

; DE runs across a screen row, H selects its font page. Only the first
; and last screen bytes are masked. Completed interior bytes are stored
; directly from the previous glyph's spill and the current glyph's head.
    MACRO GRAPH_STREAM label,shift,inverse
label:
    IF GRAPH_PRE_MODE
    ld a,h
    add a,GRAPH_PRE_MODE*8
    ld (.fontEnd+1),a
    ENDIF
    ; Patch the selected phase once. D is the video column in every row.
    IFDEF FONT_GRAPHIC_PAIR
    ld a,(graphPairEnd)
    ld (.pairLimit+1),a
    IFDEF FONT_GRAPHIC_QUAD
    ld a,(graphQuadEnd)
    ld (.quadLimit+1),a
    ENDIF
    ELSE
    ld a,(graphEnd)
    ld (.end+1),a
    ENDIF
.row:
    ld a,(graphStart)
    ld d,a
    ld a,(de)
    IF GRAPH_PRE_MODE == 2 && inverse
    cpl
    ENDIF
    and 256-(256>>shift)
    ld (.carry+1),a
    IFDEF FONT_GRAPHIC_PAIR
    ld a,h
    ld hl,(fast6.text)
    ld sp,hl
    ld h,a
    IFDEF FONT_GRAPHIC_QUAD
    ld a,(graphQuadEnd)
    cp d
    ld a,(.carry+1)
    jp z,.pair
.quad:
    GRAPH_PAIR_FROM_A shift,inverse
    GRAPH_PAIR_FROM_A shift,inverse
    ld c,a
    ld a,d
.quadLimit: cp 0
    ld a,c
    jp nz,.quad
    ld (.carry+1),a
    ld a,(graphPairEnd)
    cp d
    jp z,.odd
    ENDIF
.pair:
    IF GRAPH_PRE_MODE == 2
    ld a,(.carry+1)
    GRAPH_PAIR_FROM_A shift,inverse
    ELSE
    pop bc
    ld l,c
    ld a,(hl)
    GRAPH_ROTATE shift,inverse
    ld c,a
    and 255>>shift
.carry:
    or 0
    ld (de),a
    inc d
    GRAPH_SECOND shift,inverse
    ENDIF
    ld (.carry+1),a
    ld a,d
.pairLimit: cp 0
    jp nz,.pair
.odd:
    ld a,(graphEnd)
    cp d
    jp z,.tail
    ld a,(graphLast)
    ld l,a
    IF GRAPH_PRE_MODE == 2
    ld a,(.carry+1)
    GRAPH_FULL_GLYPH inverse
    ELSE
    ld a,(hl)
    GRAPH_ROTATE shift,inverse
    ld b,a
    and 255>>shift
    ld c,a
    ld a,(.carry+1)
    or c
    ld (de),a
    inc d
    ld a,b
    and 256-(256>>shift)
    ENDIF
    ld (.carry+1),a
.tail:
    ; No stack operations or calls until SP is restored.
    ELSE
    push hl
    ld hl,(fast6.text)
    ld b,h
    ld c,l
    pop hl
.byte:
    ld a,(bc)
    inc bc
    ld l,a
    ld a,(hl)
    GRAPH_ROTATE shift,inverse
    ld l,a
    and 255>>shift
.carry:
    or 0
    ld (de),a
    inc d
    ld a,l
    and 256-(256>>shift)
    ld (.carry+1),a
    ld a,d
.end:
    cp 0
    jp nz,.byte
    ENDIF
    ld a,d
    cp high MX_SCREEN_END
    jp z,.nextRow
    ld a,(de)
    IF GRAPH_PRE_MODE == 2 && inverse
    cpl
    ENDIF
    and 255>>shift
    ld l,a
    ld a,(.carry+1)
    or l
    IF GRAPH_PRE_MODE == 2 && inverse
    cpl
    ENDIF
    ld (de),a
.nextRow:
    IFDEF FONT_GRAPHIC_PAIR
    ld bc,.rowReturn
    jp graphRestore
.rowReturn:
    ENDIF
    inc e
    IF GRAPH_PRE_MODE == 2
    inc h
    ENDIF
    inc h
    ld a,h
    IF GRAPH_PRE_MODE
.fontEnd: cp 0
    ELSE
    cp FONT_ADDR/256+8
    ENDIF
    jp nz,.row
    jp graphDone
    IF GRAPH_PRE_MODE == 2
.carry: DB 0,0 ; The full path stores an already separated right fragment.
    ENDIF
    ENDM

    GRAPH_STREAM graphNormal2,2,0
    GRAPH_STREAM graphNormal4,4,0
    GRAPH_STREAM graphNormal6,6,0
    GRAPH_STREAM graphInverse2,2,1
    GRAPH_STREAM graphInverse4,4,1
    GRAPH_STREAM graphInverse6,6,1
    IFDEF FONT_GRAPHIC_PAIR
graphRestore:
graphSavedSP:
    ld sp,0
    push bc
    ret
    ENDIF
    ENDIF
graphDone:
    ld a,(graphEnd)
    cp high MX_SCREEN_END
    jp z,graphWrap
    sub high MX_SCREEN_BASE
    add a,a
    add a,a
    ld b,a
    IFDEF FONT_ALIGNED_ROWS
    ld a,b
    ELSE
    ld a,(mon2_vars.cursorX)
    and 3
    or b
    ENDIF
    ld (mon2_vars.cursorX),a
    jp graphReturn
graphWrap:
    ld hl,(mon2_vars.cursorY)
    call fontLineEnd
graphReturn:
    ld hl,(graphNext)
    or a
    ret
    IFNDEF FONT_ALIGNED_ROWS
graphRoutes: DW graphNormal2,graphNormal4,graphNormal6
    DW graphInverse2,graphInverse4,graphInverse6
graphPhase: DB 0
    ENDIF
graphStart:     DB 0
graphEnd:       DB 0
graphNext:      DW 0
    IFDEF FONT_GRAPHIC_PAIR
    IFDEF FONT_GRAPHIC_QUAD
graphQuadEnd: DB 0
    ENDIF
graphPairEnd:   DB 0
graphLast:      DB 0
    ENDIF
