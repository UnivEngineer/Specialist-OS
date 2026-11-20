; The vertical track reflects exact logical line counts. Its column is reserved
; only for documents taller than the viewport. Clip text to full glyphs.
EDITOR_BAR_Y = 239
EDITOR_BAR_COLOR = 0F1h

; BC = unsigned number of separators inserted or removed.
editorAddLines:
    ld a,b
    or c
    ret z
    ld hl,(editorBarV.total)
    add hl,bc
    jp nc,editorStoreLineCount
    ld hl,0FFFFh
    jp editorStoreLineCount
editorSubtractLines:
    ld a,b
    or c
    ret z
    ld hl,(editorBarV.total)
    ld d,b
    ld e,c
    call bios_sub_hl_de
    jp c,editorLineCountFloor
    ld a,h
    or l
    jp nz,editorStoreLineCount
editorLineCountFloor:
    ld hl,1
editorStoreLineCount:
    ld (editorBarV.total),hl
    ; Changing the total changes thumb length even at the same position.
    ld a,0FFh
    ld (editorBarV.position),a
    ld (editorBarV.position+1),a
    ret

; Count CR, LF and CRLF in a bounded contiguous physical span.
editorCountSeparators:
    ld bc,0
    call compareHLDE
    ret z
editorCountSeparator:
    ld a,(hl)
    inc hl
    cp 13
    jp z,editorCountCR
    cp 10
    jp nz,editorCountNextByte
    inc bc
editorCountNextByte:
    ld a,l
    cp e
    jp nz,editorCountSeparator
    ld a,h
    cp d
    jp nz,editorCountSeparator
    ret
editorCountCR:
    inc bc
    call compareHLDE
    ret z
    ld a,(hl)
    cp 10
    jp nz,editorCountSeparator
    inc hl
    jp editorCountNextByte

; HL..DE is a nonempty logical range wholly on one side of the gap.
; Return its contribution to the line count when inserted or removed.
; A pair across either edge shares one break. Removing the range can
; also bring a previously separate CR and LF together.
editorRangeBreaks:
    push hl
    push de
    dec de
    ex de,hl
    call gapPhysical
    inc hl
    ex de,hl
    call gapPhysical
    call editorCountSeparators
    pop de
    pop hl
    call editorInsideCRLF
    jp nz,editorRangeTail
    dec bc
editorRangeTail:
    ex de,hl
    call gapRead
    cp 10
    ret nz
    call editorPrecededByCR
    jp nz,editorRangeJoin
    dec bc
editorRangeJoin:
    ex de,hl
    call editorPrecededByCR
    ret nz
    inc bc
    ret

; Z if the logical boundary HL splits CRLF. Preserve HL, BC and DE.
editorInsideCRLF:
    call gapRead
    cp 10
    ret nz
editorPrecededByCR:
    push de
    push hl
    ld hl,(v_textBeginLo)
    ex de,hl
    pop hl
    call compareHLDE
    pop de
    jp z,editorNoPrecedingByte
    dec hl
    call gapRead
    inc hl
    cp 13
    ret
editorNoPrecedingByte:
    xor a
    inc a
    ret

editorInvalidateBars:
    push af
    ld a,0FFh
    ld (editorBarV.position+1),a
    ld (editorBarH.position+1),a
    pop af
    ret

editorApplyScrollLayout:
    call editorSetScrollLayout
    ret z
    call clearTextViewport
    jp redrawPreservingCursor

editorSetScrollLayout:
    ld hl,(editorBarV.total)
    ld de,EDITOR_ROWS+1
    call compareHLDE
    ld a,(editorScreenColumns)
    ld b,a
    ld c,48
    jp c,editorBarWidthReady
    dec c
    dec b                       ; 8px font loses one glyph
    ld a,(v_charStep)
    cp 4
    jp z,editorBarWidthReady
    dec b                       ; 4px and 6px fonts lose two glyphs
editorBarWidthReady:
    ld a,c
    ld (editorCopyWidth),a
    ld a,(v_columns)
    cp b
    ld a,b
    ld (v_columns),a
    ret z
    IF EDIT_BANKED
    call ebInvalidateRows
    ENDIF
    ret

editorDrawVerticalBar:
    ld a,(bios_vars.cursorY)
    sub EDITOR_FIRST_Y
    ld de,1
editorBarRow:
    sub 10
    jp c,editorBarTop
    inc e
    jp editorBarRow
editorBarTop:
    ld hl,(v_lineNumber)
    call bios_sub_hl_de
    jp nc,editorBarTopReady
    ld hl,0
editorBarTopReady:
    ex de,hl
    ld hl,(editorBarV.position)
    call compareHLDE
    ret z
    ex de,hl
    ld (editorBarV.position),hl
    ld hl,editorBarV
    jp editorPaintBar

editorDrawDocumentBars:
    ld a,(editorCopyWidth)
    cp 48
    call nz,editorDrawVerticalBar
editorDrawHorizontalBar:
    call editorMaxPageWidth
    ld a,(v_leftColumnHi)
    ld hl,(v_leftColumn)
    or h
    or l
    jp nz,editorHorizontalVisible
    ld a,(editorLineWidthHi)
    or a
    jp nz,editorHorizontalVisible
    ld hl,(editorLineWidth)
    ld a,(v_columns)
    ld d,0
    ld e,a
    call compareHLDE
    jp c,editorHideHorizontal
    jp z,editorHideHorizontal
editorHorizontalVisible:
    ; Scroll positions are in groups of eight visual columns.
    ld a,(v_columns)
    and 0F8h
    rrca
    rrca
    rrca
    ld l,a
    ld h,0
    ld (editorBarH.visible),hl
    ld a,(editorCopyWidth)
    cp 48
    jp z,editorBarTrackReady
    ld a,47
editorBarTrackReady:
    ld hl,editorBarH.length
    cp (hl)
    jp z,editorBarTrackStable
    ld (hl),a
    call editorHideHorizontal
editorBarTrackStable:
    ld hl,(v_leftColumn)
    ld a,(v_leftColumnHi)
    call editorBarGroups
    push hl
    ld hl,(editorLineWidth)
    ld a,(v_columns)
    ld e,a
    ld d,0
    call bios_sub_hl_de
    ld a,(editorLineWidthHi)
    sbc a,0
    jp c,editorBarMinimumRange
    ld de,7
    add hl,de
    adc a,0
    call editorBarGroups
    jp editorBarRangeReady
editorBarMinimumRange:
    ld hl,0
editorBarRangeReady:
    pop de
    push de
    call compareHLDE
    jp c,editorBarRangeCompared
    ex de,hl
editorBarRangeCompared:
    ld hl,(editorBarH.visible)
    add hl,de
    ex de,hl
    ld hl,(editorBarH.total)
    call compareHLDE
    ex de,hl
    ld (editorBarH.total),hl
    pop de
    jp nz,editorBarHorizontalChanged
    ld a,(editorHShown)
    or a
    jp z,editorBarHorizontalChanged
    ld hl,(editorBarH.position)
    call compareHLDE
    ret z
editorBarHorizontalChanged:
    ex de,hl
    ld (editorBarH.position),hl
    ld a,1
    ld (editorHShown),a
    ld hl,editorBarH
editorPaintBar:
    COLOR EDITOR_BAR_COLOR
    call uiDrawScrollbar
    COLOR COLOR_TEXT
    ret
editorHideHorizontal:
    ld a,(editorHShown)
    or a
    ret z
    xor a
    ld (editorHShown),a
    COLOR COLOR_TEXT
    ld hl,09000h+EDITOR_BAR_Y
    ld bc,48*256+8
    xor a
    jp uiFillRectBytes

; A=separator, HL=next line; preserve the draw loop's registers.
; A row that fits at the left edge already carries its exact visual width.
editorRecordRowWidth:
    push af
    push bc
    push de
    push hl
    ld b,a
    ld a,(editorWholeRow)
    or a
    jp z,editorWidthRecorded
    ld a,(v_leftColumnHi)
    ld c,a
    ld a,(v_leftColumn)
    or c
    ld c,a
    ld a,(v_leftColumn+1)
    or c
    jp nz,editorMeasureFullRow
    ld a,(editorTextLength)
    ld c,a
    ld a,(editorSliceWidth)
    cp c
    jp z,editorMeasureFullRow
    ld e,c
    ld d,0
    xor a
    jp editorWidthStore
editorMeasureFullRow:
    ld a,b
    or a
    jp z,editorMeasureAtEnd
    dec hl
    dec a
    jp z,editorMeasureAtEnd
    dec hl
editorMeasureAtEnd:
    push hl
    ld hl,(editorSelectionRow)
    ex de,hl
    pop hl
    call bios_sub_hl_de
    ld b,h
    ld c,l
    ex de,hl
    call editorMeasureBytes
    ex de,hl
editorWidthStore:
    push af
    push de
    call editorRowLengthAddress
    ld de,-editorRowLengths & 0FFFFh
    add hl,de
    ld e,l
    ld d,h
    add hl,hl
    add hl,de
    ld de,editorPageWidths
    add hl,de
    pop de
    pop af
    ld (hl),e
    inc hl
    ld (hl),d
    inc hl
    ld (hl),a
editorWidthRecorded:
    pop hl
    pop de
    pop bc
    pop af
    ret

editorMaxPageWidth:
    ld hl,editorPageWidths
    ld de,0
    ld bc,EDITOR_ROWS*256
editorMaxWidthNext:
    inc hl
    inc hl
    ld a,(hl)
    cp c
    jp nz,editorMaxCompare
    dec hl
    ld a,(hl)
    cp d
    jp nz,editorMaxAtMiddle
    dec hl
    ld a,(hl)
    cp e
    inc hl
editorMaxAtMiddle:
    inc hl
editorMaxCompare:
    jp c,editorMaxContinue
    ld c,(hl)
    dec hl
    ld d,(hl)
    dec hl
    ld e,(hl)
    inc hl
    inc hl
editorMaxContinue:
    inc hl
    dec b
    jp nz,editorMaxWidthNext
    ex de,hl
    ld (editorLineWidth),hl
    ld a,c
    ld (editorLineWidthHi),a
    ret

    IF EDIT_BANKED
editorPageWidths = editorRowBuffer+96
    ASSERT editorPageWidths+EDITOR_ROWS*3 <= EDIT_TEXT_OVERLAY
    ELSE
    IF STANDARD_EDITOR
editorPageWidths: DS EDITOR_ROWS*3,0
    ENDIF
    ENDIF

; A:HL unsigned 24-bit visual column -> HL = column / 8.
editorBarGroups:
    ld b,3
editorBarGroupBit:
    or a
    rra
    ld c,a
    ld a,h
    rra
    ld h,a
    ld a,l
    rra
    ld l,a
    ld a,c
    dec b
    jp nz,editorBarGroupBit
    ret

editorBarV: UI_SCROLLBAR 376,EDITOR_TEXT_TOP,EDITOR_BAR_Y-EDITOR_TEXT_TOP,8,3,1,EDITOR_ROWS,0FFFFh
editorBarH: UI_SCROLLBAR 0,EDITOR_BAR_Y,48,8,2,0,0,0
editorCopyWidth: DB 48
editorScreenColumns: DB 64
editorLineWidth: DW 0
editorLineWidthHi: DB 0
editorHShown: DB 0
