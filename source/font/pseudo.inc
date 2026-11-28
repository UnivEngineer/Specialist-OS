; Timed text stays in RawLim. The second pass emits a byte mask for the
; margins, accumulating four codes before touching video memory.
    MACRO FONT_PSEUDO_ENTRY
fontPseudoDraw:
    push af
    push bc
    push de
fontPseudoLine:
    ld a,b
    or c
    jp z,fontPseudoDone
    push bc
    push hl
    ld hl,(mon2_vars.cursorY)
    ld a,h
    cp PSEUDO_WRAP
    call nc,biosEngineNewLine
    ld hl,(mon2_vars.cursorY)
    IFDEF PSEUDO_ROUND
    ld a,h
    and PSEUDO_ROUND
    ld h,a
    ENDIF
    ld (fontPseudoStart),hl
    ; For graphic 8x8, the last partial glyph is clipped, but consumed.
    ld a,h
    IF PSEUDO_STEP == 4
    and 0FCh
    rrca
    rrca
    ld e,a
    ld a,48
    sub e
    ld e,a
    ELSE
    IF PSEUDO_STEP == 3
    ld e,64
    or a
    jp z,fontPseudoRoomReady
    ENDIF
    ld d,a
    ld a,192
    sub d
    IF PSEUDO_STEP == 2
    rrca
    ld e,a
    ELSE
    ld e,0
fontPseudoRoom:
    sub 3
    jp c,fontPseudoRoomReady
    inc e
    jp fontPseudoRoom
fontPseudoRoomReady:
    ENDIF
    ENDIF
    ld a,b
    or a
    jp nz,fontPseudoLimit
    ld a,c
    cp e
    jp nc,fontPseudoLimit
    ld e,c
fontPseudoLimit:
    ld a,e
    ld (fontPseudoCount),a
    ld (fontPseudoLeft),a
    ld c,e
    ld b,0
    ld hl,fontPseudoHold
    ld (fontLineEnd+1),hl
    pop hl
    ld (fontPseudoText+1),hl
    call printStringRawLim
    ld hl,biosEngineNewLine
    ld (fontLineEnd+1),hl
    call fontPseudoMargins
    pop bc
    ld a,(fontPseudoCount)
    ld e,a
    ld a,c
    sub e
    ld c,a
    jp nc,fontPseudoMaybeWrap
    dec b
fontPseudoMaybeWrap:
    push hl
    ld hl,(mon2_vars.cursorY)
    ld a,h
    cp 192
    call nc,biosEngineNewLine
    pop hl
    jp fontPseudoLine
fontPseudoDone:
    pop de
    pop bc
    pop af
    ret
fontLineEnd:
    jp biosEngineNewLine
fontPseudoHold:
    ; Some packet renderers reach the edge without storing their final X.
    ld a,192
    ld (mon2_vars.cursorX),a
    ret
fontPseudoMargins:
    ld a,(mon2_vars.inverse)
    add a,0FFh
    sbc a,a
    ld (fontPseudoFill+1),a
    ld (fontPseudoBackground+1),a
    ld hl,(fontPseudoStart)
    ld a,h
    and 3
    IF PSEUDO_STEP == 4
    ld c,a
    ld b,0
    push hl
    ld hl,fontPseudoRightMasks
    add hl,bc
    ld a,(hl)
    pop hl
    ld (fontPseudoRight+1),a
    ld (fontPseudoCoverage),a
    cpl
    ld (fontPseudoLeftMask+1),a
    ELSE
    ld (fontPseudoPhase),a
    ENDIF
    ld a,h
    rrca
    rrca
    and 3Fh
    add a,high MX_SCREEN_BASE
    ld h,a
    ld (fontPseudoVideo),hl
    xor a
    ld (fontPseudoMask),a
    IF PSEUDO_STEP != 4
    ld (fontPseudoCoverage),a
    ENDIF
fontPseudoPacket:
    ld a,(fontPseudoLeft)
    cp 4
    ld b,4
    jp nc,fontPseudoPacketSize
    ld b,a
fontPseudoPacketSize:
    sub b
    ld (fontPseudoLeft),a
    ld c,b
    ld b,0
fontPseudoText:
    ld hl,0
    add hl,bc
    ld (fontPseudoText+1),hl
    ld b,c
    ld d,b
    ld c,0
fontPseudoRead:
    dec hl
    call fontPseudoClassify
    ld a,c
    rla
    ld c,a
    dec b
    jp nz,fontPseudoRead
    ld b,d
    IF PSEUDO_STEP != 4
    ld a,(fontPseudoPhase)
    or a
    jp nz,fontPseudoFlag
    ld a,b
    cp 4
    jp z,fontPseudoQuad
    ENDIF
fontPseudoFlag:
    ld a,c
    rrca
    ld c,a
    push bc
    sbc a,a
    call fontPseudoCell
    pop bc
    dec b
    jp nz,fontPseudoFlag
fontPseudoPacketDone:
    ld a,(fontPseudoLeft)
    or a
    jp nz,fontPseudoPacket
    IF PSEUDO_STEP == 4
    ld a,(fontPseudoLeftMask+1)
    ld (fontPseudoCoverage),a
    ENDIF
    call fontPseudoFlush
    ld hl,(fontPseudoText+1)
    ret

    IF PSEUDO_STEP != 4
    FONT_PSEUDO_HELPER
    ENDIF
    ENDM

    MACRO FONT_PSEUDO_HELPER
; CF marks KOI8-R box drawing or blocks. Compact has only the DEL fallback.
fontPseudoClassify:
    IFDEF PSEUDO_COMPACT
    or a
    ret
    ELSE
    ld a,(hl)
    cp 80h
    jp c,fontPseudoNo
    cp 93h
    ret c
    cp 0A0h
    jp c,fontPseudoNo
    cp 0BFh
    ret nc
    cp 0A3h
    ret z
    cp 0B3h
    jp z,fontPseudoNo
    scf
    ret
fontPseudoNo:
    or a
    ret
    ENDIF

; A=00/FF. Merge the glyph's mask into the current screen column. On
; crossing the byte boundary, commit a full column and start the next one.
    IF PSEUDO_STEP == 4
fontPseudoCell:
    ld e,a
fontPseudoRight:
    and 0
    ld hl,fontPseudoMask
    or (hl)
    ld (hl),a
    push de
    call fontPseudoFlush
    pop de
    ld a,e
fontPseudoLeftMask:
    and 0
    ld (fontPseudoMask),a
    ld a,0FFh
    ld (fontPseudoCoverage),a
    ret
fontPseudoRightMasks: DB 0FFh,03Fh,00Fh,003h
    ELSE
; Whole byte-aligned quads use one table lookup and one write per byte.
fontPseudoQuad:
    ld a,c
    add a,a
    IF PSEUDO_STEP == 3
    add a,c
    ENDIF
    ld e,a
    ld d,0
    ld hl,fontPseudoQuadMasks
    add hl,de
    ld b,PSEUDO_STEP
fontPseudoQuadByte:
    ld a,(hl)
    push bc
    push hl
    ld b,a
    ld c,0FFh
    call fontPseudoWriteColumn
    pop hl
    pop bc
    inc hl
    dec b
    jp nz,fontPseudoQuadByte
    jp fontPseudoPacketDone

; The left and right fragments handle the head and the last partial quartet.
fontPseudoCell:
    ld e,a
    ld hl,fontPseudoShapes
    ld a,(fontPseudoPhase)
    add a,a
    ld c,a
    ld b,0
    add hl,bc
    ld d,(hl)
    inc hl
    ld c,(hl)
    ld hl,fontPseudoCoverage
    ld a,d
    or (hl)
    ld (hl),a
    ld hl,fontPseudoMask
    ld a,e
    and d
    or (hl)
    ld (hl),a
    ld hl,fontPseudoPhase
    ld a,(hl)
    add a,PSEUDO_STEP
    cp 4
    jp c,fontPseudoPhaseStore
    sub 4
    ld (hl),a
    push bc
    push de
    call fontPseudoFlush
    pop de
    pop bc
    ld a,c
    ld (fontPseudoCoverage),a
    and e
    ld (fontPseudoMask),a
    ret
fontPseudoPhaseStore:
    ld (hl),a
    ret

fontPseudoShapes:
    IF PSEUDO_STEP == 2
    DB 0F0h,0, 03Ch,0, 00Fh,0, 003h,0C0h
    ELSE
    IF PSEUDO_STEP == 3
    DB 0FCh,0, 03Fh,0, 00Fh,0C0h, 003h,0F0h
    ELSE
    DB 0FFh,0, 03Fh,0C0h, 00Fh,0F0h, 003h,0FCh
    ENDIF
    ENDIF
fontPseudoQuadMasks:
pseudoIndex = 0
    DUP 16
    IF PSEUDO_STEP == 2
    DB ((pseudoIndex&1)*0F0h)+(((pseudoIndex>>1)&1)*0Fh)
    DB (((pseudoIndex>>2)&1)*0F0h)+(((pseudoIndex>>3)&1)*0Fh)
    ELSE
    DB ((pseudoIndex&1)*0FCh)+(((pseudoIndex>>1)&1)*3)
    DB (((pseudoIndex>>1)&1)*0F0h)+(((pseudoIndex>>2)&1)*0Fh)
    DB (((pseudoIndex>>2)&1)*0C0h)+(((pseudoIndex>>3)&1)*03Fh)
    ENDIF
pseudoIndex = pseudoIndex+1
    EDUP
    ENDIF
    ENDM

    MACRO FONT_PSEUDO_TAIL
    IF PSEUDO_STEP == 4
    FONT_PSEUDO_HELPER
    ENDIF
fontPseudoFlush:
    ld a,(fontPseudoCoverage)
    or a
    ret z
    ld c,a
    ld a,(fontPseudoMask)
    ld b,a
fontPseudoWriteColumn:
    ld hl,(fontPseudoVideo)
    ld a,h
    cp high MX_SCREEN_END
    ret nc
    inc h
    ld (fontPseudoVideo),hl
    dec h
    ld d,h
    ld e,l
    ld a,l
    cp 0FFh
    jp z,fontPseudoTop
    inc l
    call fontPseudoEdge
fontPseudoTop:
    ld a,e
    sub 8
    ret c
    ld l,a
    inc a
    ld e,a
fontPseudoEdge:
    ld a,(de)
fontPseudoFill:
    xor 0
    and b
fontPseudoBackground:
    xor 0
    inc c
    jp z,fontPseudoFullByte
    dec c
    xor (hl)
    and c
    xor (hl)
    ld (hl),a
    ret
fontPseudoFullByte:
    dec c
    ld (hl),a
    ret
fontPseudoStart:        DW 0
fontPseudoCount:        DB 0
fontPseudoVideo:        DW 0
fontPseudoPhase:        DB 0
fontPseudoLeft:         DB 0
fontPseudoMask:         DB 0
fontPseudoCoverage:     DB 0
    ENDM
