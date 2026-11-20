; The HEX view addresses the common gap buffer by file offset.
; A physical screen row contains 16 bytes, or 8 in the split layout.
hexView:
    xor a
    ld (hexNibble),a
    call hexRawView
    ld a,(editorPendingHex)
    or a
    jp z,hexViewToolbar
    xor a
    ld (editorPendingHex),a
    call hexTryContainer
    call c,hexRawView
hexViewToolbar:
    call editorToolbar
hexGeometry:
    call hexPrepareGeometry
    jp hexUpdate
hexPrepareGeometry:
    ld a,(v_charStep)
    cp 4
    jp z,hexWideBytes
hexNarrowBytes:
    ld b,17
    ld a,16
    jp hexSetWidth
hexWideBytes:
    ld b,33
    ld a,8
hexSetWidth:
    ld (hexAcross),a
    ld a,b
    ld (hexBlockRows),a
    ld a,(hexAcross)
    dec a
    ld (hexByteMask),a
    ld a,(hexLoad)
    and 15
    ld (hexAlign),a
    call hexHeaderGeometry
    ld b,EDITOR_ROWS
    ld c,EDITOR_TEXT_TOP
    ld a,(hexViewType)
    or a
    call nz,hexReserveHeader
    ld a,(hexAcross)
    cp 8
    call z,hexReserveHeader
    ld a,b
    ld (hexBar.visible),a
    ld a,c
    ld (hexBar.y),a
    add a,8
    ld (hexFirstY),a
    ld a,EDITOR_BAR_Y
    sub c
    ld (hexBar.length),a
    ld hl,0FFFFh
    ld (hexFinalSummary),hl
    ld (hexPrintedCaret),hl
    ld hl,(hexSize)
    call hexDataRow
    inc hl                       ; one caret row for the EOF position
    push hl
    ld hl,(hexSize)
    call hexGrid
    ld a,l
    or a
    pop hl
    jp z,hexRowsReady             ; a full block already has its checksum row
    ld (hexFinalSummary),hl
    ld b,a
    ld a,(hexByteMask)
    and b
    jp nz,hexPartialRow
    dec hl                       ; a row-aligned EOF follows the block sum
    ld (hexFinalSummary),hl
    inc hl
hexPartialRow:
    inc hl
hexRowsReady:
    ld (hexTrailerRow),hl       ; RKS adds a display-only row after the body.
    ld a,(hexViewType)
    cp 2
    jp nz,hexStoreRows
    inc hl
hexStoreRows:
    ld (hexBar.total),hl
    ld hl,(hexTop)
    call hexSetTop
    call hexSetGoal
    jp hexRefresh
hexUpdate:
    call editorUpdateMark
    ld a,(hexMarkDirty)
    or a
    call nz,hexPaintChangedMark
    call hexEnsureVisible
    ld a,(hexDirtyScreen)
    or a
    call nz,hexDrawPage
    call hexDrawAddress
    call hexPlaceCursor
hexWait:
    call editorGetch
    jp c,hexControl
    cp 4
    call c,hexCheckBlockKey
    cp 8Ah
    ret z
    cp 8Bh
    jp z,hexExit
    cp 1Fh
    jp z,hexExit
    cp 1
    jp z,hexHelp
    cp 3
    jp z,hexMark
    cp 4
    jp z,hexSaveKey
    cp 5
    jp z,hexFilterKey
    cp 6
    jp z,hexContainerKey
    cp 7
    jp z,hexRecalcContainer
    cp 08h
    jp z,hexLeft
    cp 18h
    jp z,hexRight
    cp 19h
    jp z,hexUp
    cp 1Ah
    jp z,hexDown
    cp 0Ch
    jp z,hexHome
    cp 0Ah
    jp z,hexEnd
    jp hexInputDigit
hexExit:
    call keyHandler_CTP
    jp hexResumeModal
hexHelp:
    call editorHelp
    jp hexResumeModal
hexSaveKey:
    call editorWriteDocument
hexResumeModal:
    ld hl,0FFFFh
    ld (hexCachedLine),hl
    jp hexUpdate
hexMark:
    ld a,(editorMarkActive)
    or a
    call z,hexEraseMark
    call editorToggleMark
    xor a
    ld (hexDirtyScreen),a
    jp hexUpdate
hexEraseMark:
    ld hl,(v_selectionBegin)
    ld a,h
    or l
    ret z
    ld a,0FFh
    ld (hexDamageCol),a
    jp hexDrawRowsOnly
hexFilterKey:
    ld hl,hexFiltered
    ld a,(hl)
    ld (hexOldFilter),a
    ld a,(v_charStep)
    cp 3
    jp z,hexCycle6
    ld a,(hl)
    dec a
    cp 0FEh
    jp nz,hexFilterChanged
    ld a,1
    jp hexFilterChanged
hexCycle6:
    ld hl,hexMode6
    ld a,(hl)
    inc a
    and 3
    ld (hl),a
    call hexSetLayout
    ld a,(hexMode6)
    ld e,a
    ld d,0
    ld hl,hexFilters6
    add hl,de
    ld a,(hl)
    ld (hexFiltered),a
    ld a,(hexMode6)
    cp 2
    ld a,(hexTextCol)
    jp nc,hexDrawChangedField ; KOI-to-KOI: repaint only changed glyphs.
    ld hl,0FFFFh
    ld (hexPrintedCaret),hl
    xor a                     ; CS and KOI use different byte columns.
    jp hexDrawChangedField
hexFilterChanged:
    ld (hexFiltered),a
    ld a,(hexTextCol)
hexDrawChangedField:
    ld (hexDamageCol),a
    IF EDIT_BANKED
    call hexCacheReset
    ENDIF
    call hexDrawRuler
    call hexDrawRowsOnly
    call hexDrawAddress
    call hexPlaceCursor
    jp hexWait
hexDrawRowsOnly:
    IF EDIT_BANKED
    ld a,(hexDamageCol)
    or a
    call z,hexPacketEnable
    ENDIF
    ld a,EDITOR_LAST_Y+1
    ld (hexDrawLimit),a
    call hexRedrawRows
    xor a
    ld (hexDamageCol),a
    ret
hexRepaint:
    call hexRefresh
    jp hexUpdate
hexRefresh:
    call hexSetLayout
    IF EDIT_BANKED
    call hexCacheReset
    ENDIF
    ld hl,0FFFFh
    ld (hexCachedLine),hl
    xor a
    ld (hexMarkDirty),a
    ld (hexDamageCol),a
    ld a,1
    ld (hexDirtyScreen),a
    ret
; A moving mark only changes bytes between the old and new carets. Update
; those rows on the old bitmap before the scroll copies its retained portion.
hexMarkMoved:
    ld a,1
    ld (hexMarkDirty),a
    ret
hexPaintChangedMark:
    xor a
    ld (hexMarkDirty),a
    ld a,(hexDirtyScreen)
    dec a
    ret z
    ld hl,(hexCaret)
    call hexFileRow
    ex de,hl
    ld hl,(hexCaretRow)
    call compareHLDE
    jp c,hexMarkSorted
    ex de,hl
hexMarkSorted:
    push de
    ex de,hl
    ld hl,(hexPaintedTop)
    call compareHLDE
    jp nc,hexMarkStart
    ex de,hl
hexMarkStart:
    push hl
    ld hl,(hexPaintedTop)
    ex de,hl
    pop hl
    push hl
    call bios_sub_hl_de
    ld a,h
    or a
    jp nz,hexMarkOutside
    ld a,l
    ld hl,hexBar.visible
    cp (hl)
    jp nc,hexMarkOutside
    call hexScreenY
    ld (hexY),a
    pop hl
    call hexDecodeRow
    pop de
    ld a,(hexY)
    push af
    ld hl,(hexPaintedTop)
    ex de,hl
    call bios_sub_hl_de
    ld a,h
    or a
    jp nz,hexMarkLastRow
    ld a,l
    ld hl,hexBar.visible
    cp (hl)
    jp c,hexMarkBottom
hexMarkLastRow:
    ld a,(hexBar.visible)
    dec a
hexMarkBottom:
    call hexScreenY
    inc a
    ld (hexDrawLimit),a
    pop af
    jp hexDrawNext
hexMarkOutside:
    pop hl
    pop hl
    ret
hexScreenY:
    ld l,a
    add a,a
    add a,a
    add a,l
    add a,a
    ld hl,hexFirstY
    add a,(hl)
    ret
; Every layout fills 48 bitmap columns, using the text editor's toolbar.
hexKeys:
    E_KEY 'F2','Help'
    UI_TOOLBAR_FIELD 2,0,'F4'
    DB 084h
hexMarkLabel: DB 'Mark',0
    E_KEY 'F5','Save'
    E_KEY 'F6','KOI'
    E_KEY 'F7','Exec'
    UI_TOOLBAR_FIELD 2,0,'F8'
    UI_TOOLBAR_FIELD 3,080h,'CS'
    E_KEY 'F9','Text'
    UI_TOOLBAR_FIELD 3,0,'F10'
    UI_TOOLBAR_FIELD 4,080h,'Exit'
    DB 0

hexControl:
    cp 0Ch
    jp z,hexFirst
    cp 0Ah
    jp z,hexLast
    cp 19h
    jp z,hexPageUp
    cp 1Ah
    jp z,hexPageDown
    jp hexWait
hexLeft:
    ld hl,(hexCaret)
    ld a,(hexNibble)
    or a
    jp nz,hexClearNibble
    ld a,h
    or l
    jp z,hexWait
    dec hl
    jp hexSetLowNibble
hexRight:
    ld hl,(hexCaret)
    ld a,(hexNibble)
    or a
    jp nz,hexNextByte
    call hexClamp
    jp nc,hexWait
hexSetLowNibble:
    ld a,1
    jp hexStoreHorizontal
hexNextByte:
    inc hl
    jp hexHorizontal
hexHome:
    ld hl,(hexCaret)
    call hexGrid
    ld a,(hexByteMask)
    cpl
    and l
    ld l,a
    jp hexSectionMove
hexEnd:
    ld hl,(hexCaret)
    call hexGrid
    ld a,(hexByteMask)
    or l
    ld l,a
hexSectionMove:
    call hexBoundSection
    call hexGridFile
    jp hexHorizontal
hexFirst:
    ld hl,0
    jp hexHorizontal
hexLast:
    ld hl,(hexSize)
hexHorizontal:
    call hexClamp
hexClearNibble:
    xor a
hexStoreHorizontal:
    ld (hexNibble),a
    ld (hexCaret),hl
    call hexSetGoal
    jp hexUpdate
hexUp:
    ld hl,(hexCaret)
    call hexFileRow
    ld a,h
    or l
    jp z,hexWait
    dec hl
    call hexDecodeRow
    ld a,(hexSummary)
    or a
    jp z,hexPageColumn
    ld hl,(hexDisplayRow)
    dec hl
    jp hexMoveToRow
hexDown:
    ld hl,(hexCaret)
    call hexFileRow
    inc hl
    jp hexPageTarget
hexPageUp:
    ld hl,(hexTop)
    ld de,EDITOR_PAGE_STEP
    call bios_sub_hl_de
    jp nc,hexPageUpTop
    ld hl,0
hexPageUpTop:
    call hexSetTop
    ld hl,(hexCaretRow)
    ld de,EDITOR_PAGE_STEP
    call bios_sub_hl_de
    jp nc,hexPageTarget
    ld hl,0
    jp hexPageTarget
hexPageDown:
    ld hl,(hexTop)
    ld de,EDITOR_PAGE_STEP
    add hl,de
    call hexSetTop
    ld hl,(hexCaretRow)
    ld de,EDITOR_PAGE_STEP
    add hl,de
hexPageTarget:
    call hexDecodeRow
    ld a,(hexSummary)
    or a
    jp z,hexPageColumn
    ld hl,(hexDisplayRow)
    inc hl
hexMoveToRow:
    call hexDecodeRow
hexPageColumn:
    ex de,hl
    ld hl,(hexSize)
    call compareHLDE
    ex de,hl
    jp z,hexVerticalStore
    call hexGrid
    ld a,(hexByteMask)
    cpl
    and l
    ld l,a
    ld a,(hexGoal)
    or l
    ld l,a
    call hexBoundSection
    call hexGridFile
hexVerticalStore:
    call hexClamp
    ld (hexCaret),hl
    jp c,hexUpdate
    xor a
    ld (hexNibble),a
    jp hexUpdate

; Insert one checksum row after every 256 bytes. At a row-aligned final
; partial block, EOF follows its checksum instead of sharing that row.
hexFileRow:
    push hl
    ex de,hl
    ld hl,(hexSize)
    call compareHLDE
    pop hl
    jp nz,hexDataRow
    push hl
    call hexGrid
    ld a,l
    ld b,a
    ld a,(hexByteMask)
    and l
    pop hl
    jp nz,hexDataRow
    ld a,b
    or a
    jp z,hexDataRow
    call hexDataRow
    inc hl
    ret
hexDataRow:
    call hexGrid
    push de
    push hl
    call hexDivideRow
    pop de
    ld e,d
    ld d,0
    add hl,de
    pop de
    ld a,e
    or a
    ret z
    ld a,(hexHeaderRows)
    ld e,a
    ld d,0
    call hexSkipHalf
    jp z,hexBodyBias
    dec e
hexBodyBias:
    add hl,de
    ret
hexDivideRow:
    ld a,(hexAcross)
    cp 8
    call nz,hexShiftRight
    call hexShiftRight
    call hexShiftRight
    jp hexShiftRight
hexShiftRight:
    ld a,h
    or a
    rra
    ld h,a
    ld a,l
    rra
    ld l,a
    ret
hexEnsureVisible:
    ld hl,(hexCaret)
    call hexFileRow
    ld (hexCaretRow),hl
    ex de,hl
    ld hl,(hexTop)
    call compareHLDE
    jp c,hexCheckBottom
    ex de,hl
    jp nz,hexSetTop
    ret
hexCheckBottom:
    ; Keep the final checksum visible when EOF shares its last data row.
    push de
    ld hl,(hexCaret)
    ex de,hl
    ld hl,(hexSize)
    call compareHLDE
    pop de
    jp nz,hexCheckTop
    push de
    call hexGrid
    ld a,l
    pop de
    ld hl,hexByteMask
    and (hl)
    jp z,hexCheckTrailer
    inc de
hexCheckTrailer:
    ld a,(hexViewType)
    cp 2
    jp nz,hexCheckTop
    ld hl,(hexTrailerRow)
    ex de,hl
hexCheckTop:
    ld hl,(hexTop)
    ld a,(hexBar.visible)
    ld c,a
    ld b,0
    add hl,bc
    call compareHLDE
    jp z,hexScrollToBottom
    ret nc
hexScrollToBottom:
    ex de,hl
    ld a,(hexBar.visible)
    ld e,a
    ld d,0
    dec de
    call bios_sub_hl_de
hexNewTop:
hexSetTop:
    push hl
    ld hl,(hexBar.total)
    ld a,(hexBar.visible)
    ld e,a
    ld d,0
    call bios_sub_hl_de
    jp nc,hexTopLimit
    ld hl,0
hexTopLimit:
    ex de,hl
    pop hl
    call compareHLDE
    jp c,hexTopInRange
    ex de,hl
hexTopInRange:
    ex de,hl
    ld hl,(hexTop)
    call compareHLDE
    ret z
    ex de,hl
    ld (hexTop),hl
    ld a,(hexDirtyScreen)
    or a
    ret nz
    ld a,2                   ; only the viewport moved
    ld (hexDirtyScreen),a
    ret

; Decode a display row inside a 17- or 33-row block.
hexDecodeRow:
    ld (hexDisplayRow),hl
    xor a
    ld (hexSummary),a
    ex de,hl
    ld hl,(hexBar.total)
    call compareHLDE
    jp c,hexDecodeEOF
    jp z,hexDecodeEOF
    ld hl,(hexTrailerRow)
    call compareHLDE
    jp z,hexDecodeTrailer
    ld hl,(hexFinalSummary)
    call compareHLDE
    jp c,hexDecodeEOF
    jp z,hexDecodeFinal
    ex de,hl
    ld a,(hexHeaderRows)
    ld e,a
    ld d,0
    call bios_sub_hl_de
    jp c,hexDecodeHeader
    call hexSkipHalf
    jp z,hexDecodeStartBlock
    inc hl
hexDecodeStartBlock:
    ld a,(hexBlockRows)
    ld e,a
    ld d,0
    ld b,0
hexDecodeBlock:
    call bios_sub_hl_de
    jp c,hexDecodeRemainder
    inc b
    jp hexDecodeBlock
hexDecodeRemainder:
    add hl,de
    dec e
    ld a,l
    cp e
    jp z,hexDecodeSummary
    add a,a
    add a,a
    add a,a
    ld l,a
    ld a,(hexAcross)
    cp 8
    ld a,l
    jp z,hexDecodeByte
    add a,a
hexDecodeByte:
    ld l,a
    ld h,b
    jp hexDecodeBody
hexDecodeFinal:
    ld hl,(hexSize)
    call hexGrid
    ld b,h
hexDecodeSummary:
    ld a,1
    ld (hexSummary),a
    ld h,b
    ld l,0
hexDecodeBody:
    ld a,(hexBase)
    ld e,a
    ld d,0
    call hexGridFile
    jp hexSetOffset
hexDecodeHeader:
    add hl,de
    ld a,l
    inc a
    cp e
    jp nz,hexDecodeHeaderData
    ld a,2
    ld (hexSummary),a
    ld hl,0
    jp hexSetOffset
hexDecodeHeaderData:
    add hl,hl
    add hl,hl
    add hl,hl
    ld a,(hexAcross)
    cp 8
    jp z,hexSetOffset
    add hl,hl
    jp hexSetOffset
hexDecodeTrailer:
    ld a,3
    ld (hexSummary),a
hexDecodeEOF:
    ld hl,(hexSize)
hexSetOffset:
    ld (hexOffset),hl
    ret

hexDrawPage:
    IF EDIT_BANKED
    call hexPacketEnable
    ENDIF
    ld hl,(hexPaintedTop)
    ex de,hl
    ld hl,(hexTop)
    ld (hexPaintedTop),hl
    call bios_sub_hl_de
    ld a,(hexDirtyScreen)
    ld b,a
    xor a
    ld (hexDirtyScreen),a
    ld a,EDITOR_LAST_Y+1
    ld (hexDrawLimit),a
    ld a,b
    cp 1
    jp z,hexFullPage
    cp 3
    jp z,hexReformatPage
    cp 4
    jp z,hexRedrawRows
    ; Retain every overlapping physical row, including split byte rows.
    ; BC receives the first new row's Y offset in pixels below the text top.
    ld a,h
    or a
    jp z,hexScrollForward
    ex de,hl
    ld hl,0
    call bios_sub_hl_de
    call hexScrollDistance
    jp nc,hexRedrawRows
    ld b,090h
    ld a,(hexBar.y)
    add a,c
    ld c,a
    ld a,(hexBar.y)
    ld l,a
    ld h,090h
    push de
    call hexCopyScreen
    pop de
    ld a,EDITOR_HEIGHT+EDITOR_FIRST_Y
    sub e
    ld (hexDrawLimit),a
    jp hexRedrawRows
hexScrollForward:
    call hexScrollDistance
    jp nc,hexRedrawRows
    ld a,(hexBar.y)
    add a,c
    ld h,090h
    ld l,a
    ld a,(hexBar.y)
    ld c,a
    ld b,090h
    push de
    call hexCopyScreen
    pop de
    ld a,(hexFirstY)
    add a,e
    push af
    ld hl,(hexTop)
    ld a,(hexShiftRows)
    ld e,a
    ld d,0
    add hl,de
    call hexDecodeRow
    pop af
    jp hexDrawNext
; HL=absolute distance. Return C=exposed pixels, E=retained pixels, carry if overlap.
hexScrollDistance:
    ld a,h
    or a
    ret nz
    ld a,l
    or a
    ret z
    ld b,l
    ld hl,hexBar.visible
    cp (hl)
    ret nc
    add a,a
    add a,a
    add a,b
    add a,a
    ld c,a
    ld a,(hl)
    sub b
    ld (hexShiftRows),a
    ld a,(hexFirstY)
    ld e,a
    ld a,EDITOR_LAST_Y+10
    sub e
    sub c
    ld e,a
    scf
    ret
hexCopyScreen:
    COLOR COLOR_TEXT
    ld d,47
    IF EDIT_HEX_PSEUDO
    ; The separator glyphs occupy the outer two rows as well.
    jp uiCopyRect
    ELSE
    jp uiCopyTextRect
    ENDIF
hexFullPage:
    ld hl,0FFFFh
    ld (hexPrintedCaret),hl
    COLOR COLOR_TEXT
    ld hl,09000h
    ld b,48
    ld c,247
    xor a
    call uiFillRectBytes
    jp hexPageHeader
hexReformatPage:
    ; F7 paints the rows in place. Only the old banner stripe and track
    ; extend beyond the new glyph rows.
    COLOR COLOR_TEXT
    ld a,(hexBar.y)
    ld l,a
    ld h,0BFh
    ld a,EDITOR_BAR_Y
    sub l
    ld c,a
    ld b,1
    xor a
    call uiFillRectBytes
    ld a,(hexViewType)
    or a
    jp nz,hexPageHeader
    ld a,(hexBar.y)
    ld l,a
    ld h,090h
    ld bc,48*256+10
    xor a
    call uiFillRectBytes
hexPageHeader:
    COLOR COLOR_STATUSBAR
    ld hl,09000h
    ld a,(hexBar.y)
    ld c,a
    ld b,48
    ld a,255
    call uiFillRectBytes
    call hexDrawContainer
    call hexDrawRuler
hexRedrawRows:
    ld hl,(hexTop)
    call hexDecodeRow
    ld a,(hexFirstY)
hexDrawNext:
    ld (hexY),a
    ld hl,(hexDisplayRow)
    ex de,hl
    ld hl,(hexBar.total)
    call compareHLDE
    jp c,hexDrawBar
    jp z,hexDrawBar
    IF EDIT_BANKED
    call hexPacketTryCache
    jp c,hexAdvanceRow
    ENDIF
    ld a,(hexSummary)
    or a
    jp nz,hexDrawBlock
    call hexSetCount
    or a
    jp z,hexEOF
    ld a,(hexDamageCol)
    inc a
    jp z,hexPaintOnly
    call hexSelectLine
    ld a,(hexDamageCol)
    or a
    jp nz,hexChangedText
    call hexFormatData
    call hexPrintRow
hexPaintOnly:
    call hexPaintMark
    jp hexAdvanceRow
hexEOF:
    ld a,(hexDamageCol)
    or a
    jp nz,hexDrawBar
    call hexMakeEOF
    call hexPrintRow
    jp hexAdvanceRow
    IF !EDIT_BANKED
    INCLUDE "hex-eof.inc"
    ENDIF
hexDrawBlock:
    ld a,(hexDamageCol)
    or a
    jp nz,hexAdvanceRow
    call hexFormatBlock
    call hexPrintRow
    jp hexAdvanceRow
hexChangedText:
    call hexPatchText
hexAdvanceRow:
    ld a,(hexY)
    add a,10
    ld hl,hexDrawLimit
    cp (hl)
    jp nc,hexDrawBar
    push af
    ld hl,(hexDisplayRow)
    inc hl
    call hexDecodeRow
    pop af
    jp hexDrawNext
hexDrawBar:
    IF EDIT_BANKED
    call hexPacketFlush
    xor a
    ld (hexPacketActive),a
    ENDIF
    call hexClearVacatedRows
    ld a,(hexDamageCol)
    or a
    ret nz
    ld hl,(hexBar.total)
    ld a,(hexBar.visible)
    ld e,a
    ld d,0
    call compareHLDE
    ret c
    ret z
    ld hl,(hexTop)
    ld (hexBar.position),hl
    ld hl,hexBar
    call editorPaintBar
    ret

; Exclusive Y of the last content row in the current geometry.
hexViewportEnd:
    ld hl,(hexTop)
    ex de,hl
    ld hl,(hexBar.total)
    call bios_sub_hl_de
    ld a,(hexBar.visible)
    ld b,a
    ld a,h
    or a
    ld a,b
    jp nz,hexViewportY
    cp l
    jp c,hexViewportY
    ld a,l
hexViewportY:
    call hexScreenY
    sub 8
    ret
hexClearVacatedRows:
    ld a,(hexReformatEnd)
    or a
    ret z
    push af
    xor a
    ld (hexReformatEnd),a
    call hexViewportEnd
    ld l,a
    pop af
    sub l
    ret c
    ret z
    ld c,a
    ld h,090h
    ld b,47
    xor a
    jp uiFillRectBytes
; Every HEX draw supplies absolute coordinates. In BANK queue a glyph string
; without simulating the cursor through each glyph of a completed row.
hexSetRowY:
    ld a,(hexY)
    ld (bios_vars.cursorY),a
    ret
hexPrintRow:
    IF EDIT_BANKED
    ld a,(hexPacketActive)
    or a
    jp nz,hexPacketRow
    ENDIF
    call hexSetRowY
    xor a
    ld (bios_vars.cursorX),a
    ld a,(hexLineLen)
    ld c,a
    ld b,0
    ld hl,hexRow
    jp hexPrintAt
hexPrintSpan:
    push bc
    push af
    ld a,(hexY)
    ld (bios_vars.cursorY),a
    pop af
    ld l,a
    push af
    call editorCursorX
    pop af
    call hexRowCell
    pop bc
    ld b,0
hexPrintAt:
    IF EDIT_BANKED
    IF EDIT_HEX_PSEUDO
    ld a,GFX_TEXT_PSEUDO
    ELSE
    ld a,GFX_TEXT
    ENDIF
    ld (ebQOperation),a
    jp ebQueueCommand
    ELSE
    IF EDIT_HEX_PSEUDO
    jp bios_printStringRawPseudo
    ELSE
    jp bios_printStringRawLim
    ENDIF
    ENDIF

; A contiguous logical span can be rendered directly in the document bank.
; Only the one line straddling the gap needs a 16-byte copy.
hexSelectLine:
    ld hl,(hexOffset)
    call hexGrid
    ld a,l
    and 0F0h
    ld l,a
    call hexGridFile
    ex de,hl
    ld hl,(hexCachedLine)
    call compareHLDE
    ret z
    ex de,hl
    ld (hexCachedLine),hl
    call hexGrid
    ld a,l
    and 15
    ld c,a
    ld a,16
    sub c
    ld c,a
    ld b,0
    ld hl,(hexCachedLine)
    call hexSpan
    ld a,e
    ld (hexLineCount),a
    jp nc,hexLineReady
    ld b,e
    ld de,hexScratch
hexJoinLine:
    call gapRead
    ld (de),a
    inc hl
    inc de
    dec b
    jp nz,hexJoinLine
    ld hl,hexScratch
hexLineReady:
    ld (hexLineBytes),hl
    ret

; HL=file offset at/before EOF, BC=requested count. Return DE=actual count
; and HL=physical start, or CF=1 and HL=logical start if the gap splits it.
hexSpan:
    push hl
    call hexRemaining
    ld d,b
    ld e,c
    call compareHLDE
    jp nc,hexSpanLength
    ex de,hl
hexSpanLength:
    pop bc
    ld hl,(v_textBeginLo)
    add hl,bc
    push hl
    ld b,d
    ld c,e
    ex de,hl
    ld hl,(v_gapLo)
    call bios_sub_hl_de
    jp c,hexSpanSuffix
    ld a,h
    or l
    jp z,hexSpanSuffix
    ld d,b
    ld e,c
    call compareHLDE
    pop hl
    ret
hexSpanSuffix:
    pop hl
    ld d,b
    ld e,c
    call gapPhysical
    or a
    ret

; Layout descriptors: row width, checksum/text columns (0 = hidden), flag
; for uniform three-column byte spacing, byte columns, then separator columns.
hexSetLayout:
    ld a,(v_charStep)
    cp 2
    ld hl,hexLayout4
    jp z,hexUseLayout
    cp 4
    ld hl,hexLayout8
    jp z,hexUseLayout
    ld hl,hexLayoutCS6
    ld a,(hexMode6)
    or a
    jp z,hexUseLayout
    ld hl,hexLayoutKOI6
hexUseLayout:
    ld a,(hl)
    ld (hexLineLen),a
    inc hl
    ld a,(hl)
    ld (hexSumCol),a
    inc hl
    ld a,(hl)
    ld (hexTextCol),a
    inc hl
    ld a,(hl)
    ld (hexUniformBytes),a
    inc hl
    ld (hexColumns),hl
    IF EDIT_HEX_PSEUDO
    ld a,(hexAcross)
    ld e,a
    ld d,0
    add hl,de
    ld (hexRules),hl
    ENDIF
    ret
hexColumn:
    ld e,a
    ld d,0
    ld hl,(hexColumns)
    add hl,de
    ld a,(hl)
    ret
hexBlankRow:
    HEX_ROW_TARGET
    ld a,(hexLineLen)
    ld b,a
hexFillSpaces:
    ld a,' '
hexBlankLoop:
    ld (hl),a
    inc hl
    dec b
    jp nz,hexBlankLoop
    ret
hexRowCell:
    ld e,a
    ld d,0
    HEX_ROW_TARGET
    add hl,de
    ret
hexSetCount:
    ld hl,(hexOffset)
    call hexGrid
    ld a,(hexByteMask)
    and l
    ld (hexLead),a
    ld hl,(hexOffset)
    call hexRemaining
    ld a,0
    jp c,hexCountReady
    ld a,(hexAcross)
    ld b,a
    ld a,(hexLead)
    ld c,a
    ld a,b
    sub c
    ld b,a
    ld a,h
    or a
    ld a,b
    jp nz,hexCountReady
    cp l
    jp c,hexCountReady
    ld a,l
hexCountReady:
    ld (hexCount),a
    ret
hexFormatData:
    call hexBlankRow
    ld hl,(hexOffset)
    ld a,(hexAcross)
    cp 8
    jp nz,hexAddress
    call hexFirstHalf
    jp nz,hexNoAddress
hexAddress:
    ld hl,(hexOffset)
    call hexLoadAddress
    ld a,e
    and 0F0h
    ld e,a
    HEX_ROW_TARGET
    call hexWord
hexNoAddress:
    call hexSource
    ld a,(hexCount)
    or a
    jp z,hexFormatTail
    ld (hexSlotsLeft),a
    ld b,a
    ld a,(hexUniformBytes)
    or a
    jp z,hexGroupedBytes
    push hl
    call hexRowColumns
    ld a,(hl)
    call hexRowCell
    pop de
hexSpacedBytes:
    ld a,(de)
    inc de
    call hexByte
    inc hl
    dec b
    jp nz,hexSpacedBytes
    jp hexBytesReady
hexGroupedBytes:
    push hl
    call hexRowColumns
    ld b,h
    ld c,l
    pop hl
    ; HL walks the source bytes, BC their display columns. The row is near.
    ld d,0
hexFormatSlot:
    ld a,(bc)
    inc bc
    ld e,a
    ld a,(hl)
    inc hl
    push hl
    HEX_ROW_TARGET
    add hl,de
    call hexByte
    ld hl,hexSlotsLeft
    dec (hl)
    pop hl
    jp nz,hexFormatSlot
hexBytesReady:
    call hexTextColumnStart
    call nz,hexFormatText
hexFormatTail:
    IF EDIT_HEX_PSEUDO
    ld a,(FONT_ENGINE_HEADER+7)
    cp 3                       ; Compact lacks box-drawing glyphs.
    ld b,081h                  ; KOI8-R vertical line
    jp nz,hexGroupRule
    ld b,'|'
hexGroupRule:
    call hexPutRuleGlyphs
    ENDIF
hexTailColumn:
    ld a,(hexSumCol)
    or a
    jp z,hexNoLineSum
    ld a,(hexAcross)
    cp 8
    jp nz,hexLogicalCS
    ld hl,(hexOffset)
    call hexGrid
    ld a,l
    and 8
    jp nz,hexLogicalCS
    ld hl,(hexCachedLine)
    call hexGrid
    ld a,l
    and 15
    ld hl,hexLineCount
    add a,(hl)
    cp 9
    jp nc,hexNoLineSum
hexLogicalCS:
    ld a,(hexLineCount)
    ld e,a
    ld d,0
    ld hl,(hexLineBytes)
    call hexSumSpan
    ld a,(hexSumCol)
    call hexRowCell
    ld d,b
    ld e,c
    call hexWord
    xor a
    ret
hexNoLineSum:
    or 1
    ret
    IF EDIT_HEX_PSEUDO
; Put B at every vertical separator column of the active layout.
hexPutRuleGlyphs:
    ld hl,(hexRules)
hexGroupLine:
    ld a,(hl)
    or a
    ret z
    inc hl
    push hl
    call hexRowCell
    ld (hl),b
    pop hl
    jp hexGroupLine
    ENDIF
; Monitor-2 checksum of a nonempty contiguous span, HL=start, DE=count.
hexSumSpan:
    push hl
    add hl,de
    dec hl
    ex de,hl
    pop hl
    IF EDIT_BANKED
    jp hexLocal.calcCS
    ELSE
    jp bios_calcCS
    ENDIF
hexWord:
    ld a,d
    call hexByte
    ld a,e
hexByte:
    push af
    rrca
    rrca
    rrca
    rrca
    call hexDigit
    pop af
hexDigit:
    and 15
    add a,090h
    daa
    adc a,040h
    daa
    ld (hl),a
    inc hl
    ret
hexPaintMark:
    ld hl,(v_selectionBegin)
    ld a,h
    or l
    ret z
    ld hl,(v_textBeginLo)
    ex de,hl
    ld hl,(hexOffset)
    add hl,de
    ex de,hl
    ld hl,(v_selectionBegin)
    call hexClipIndex
    ld (hexMarkFirst),a
    ld hl,(v_selectionEnd)
    call hexClipIndex
    ld hl,hexMarkFirst
    cp (hl)
    ret z
    ld (hexMarkEnd),a
    ld a,(hexDamageCol)
    inc a
    cp 2
    jp nc,hexMarkChars
hexMarkBytes:
    ld a,(hexMarkEnd)
    dec a
    call hexAddLead
    call hexColumn
    add a,2
    push af
    ld a,(hexMarkFirst)
    call hexAddLead
    call hexColumn
    ld l,a
    pop af
    call hexInvert
    jp hexMarkChars
hexMarkChars:
    ld a,(hexDamageCol)
    inc a
    cp 3
    jp c,hexMarkCharRange
    ld hl,hexPatchFirst
    ld a,(hexMarkFirst)
    cp (hl)
    jp nc,hexMarkClipEnd
    ld a,(hl)
    ld (hexMarkFirst),a
hexMarkClipEnd:
    ld hl,hexPatchEnd
    ld a,(hexMarkEnd)
    cp (hl)
    jp c,hexMarkClipReady
    ld a,(hl)
    ld (hexMarkEnd),a
hexMarkClipReady:
    ld hl,hexMarkFirst
    cp (hl)
    ret c
    ret z
hexMarkCharRange:
    call hexTextColumnStart
    ret z
    ld b,a
    ld a,(hexMarkFirst)
    add a,b
    ld l,a
    ld a,(hexMarkEnd)
    add a,b
hexInvert:
    sub l
    push hl
    ld l,a
    ld h,0
    call editorSelectionPixels
    push hl
    pop bc
    pop hl
    ld h,0
    push bc
    call editorSelectionPixels
    pop bc
    ld a,(hexY)
    sub 7
    ld d,a
    ld e,8
    jp uiInverseRectPixels
hexClipIndex:
    call bios_sub_hl_de
    ld a,0
    ret c
    ld a,h
    or a
    ld a,(hexCount)
    ret nz
    cp l
    ret c
    ld a,l
    ret
hexSource:
    ld hl,(hexCachedLine)
    ex de,hl
    ld hl,(hexOffset)
    call bios_sub_hl_de
    ex de,hl
    ld hl,(hexLineBytes)
    add hl,de
    ret

; Map the byte in C to a glyph in the active KOI8 font. KOI-7R has
; uppercase Cyrillic at 60h..7Fh. The document itself is never recoded.
hexGlyph:
    or a
    ld a,c
    ret z
    jp m,hexGlyph7
    cp 20h
    jp c,hexDot
    cp 7Fh
    ret c
    cp 0A3h
    ret z
    cp 0B3h
    ret z
    cp 0C0h
    ret nc
hexDot:
    ld a,'.'
    ret
hexGlyph7:
    cp 20h
    jp c,hexDot
    cp 80h
    jp nc,hexDot
    cp 60h
    ret c
    or 80h
    ret

hexFormatText:
    call hexRowCell
    push hl
    call hexSource
    pop de
    ld a,(hexCount)
    ld b,a
hexTextLoop:
    ld c,(hl)
    ld a,(hexFiltered)
    call hexGlyph
    ld (de),a
    inc de
    inc hl
    dec b
    jp nz,hexTextLoop
    ret

; F6 sends exactly the contiguous runs whose glyphs changed, including
; their selection. Unchanged glyphs never touch the graphics queue.
hexPatchText:
    ld a,(hexCount)
    or a
    ret z
    ld b,a
    call hexTextColumnStart
    call hexRowCell
    ex de,hl
    push de
    call hexSource
    pop de
    ld a,0FFh
    ld (hexPatchFirst),a
hexPatchLoop:
    ld c,(hl)
    ld a,(hexOldFilter)
    call hexGlyph
    push af
    ld a,(hexFiltered)
    call hexGlyph
    ld (de),a
    ld c,a
    pop af
    cp c
    jp nz,hexPatchDifferent
    call hexFlushGlyphs
    jp hexPatchNext
hexPatchDifferent:
    ld a,(hexCount)
    sub b
    ld c,a
    ld a,(hexPatchFirst)
    cp 0FFh
    jp nz,hexPatchLast
    ld a,c
    ld (hexPatchFirst),a
hexPatchLast:
    ld a,c
    inc a
    ld (hexPatchEnd),a
hexPatchNext:
    inc hl
    inc de
    dec b
    jp nz,hexPatchLoop
hexFlushGlyphs:
    ld a,(hexPatchFirst)
    cp 0FFh
    ret z
    push bc
    push de
    push hl
    ld hl,hexPatchFirst
    ld a,(hexPatchEnd)
    sub (hl)
    ld c,a
    call hexTextColumnStart
    ld hl,hexPatchFirst
    add a,(hl)
    call hexPrintSpan
    call hexPaintMark
    ld a,0FFh
    ld (hexPatchFirst),a
    pop hl
    pop de
    pop bc
    ret

hexFormatBlock:
    HEX_ROW_TARGET
    ld a,(hexLineLen)
    ld b,a
    ld a,(FONT_ENGINE_HEADER+7)
    cp 3                       ; Compact lacks box-drawing glyphs.
    ld a,080h
    IF EDIT_HEX_PSEUDO
    ld c,08Ah                  ; KOI8-R crossing
    ENDIF
    jp nz,hexBlockRule
    ld a,'-'
    IF EDIT_HEX_PSEUDO
    ld c,'+'
    ENDIF
hexBlockRule:
    call hexBlankLoop
    ld a,(hexSumCol)
    cp HEX_CS6_COL
    jp nz,hexBlockContent
    dec hl
    ld (hl),' '
hexBlockContent:
    IF EDIT_HEX_PSEUDO
    ld b,c
    call hexPutRuleGlyphs
    ENDIF
    ld a,(hexSummary)
    cp 2
    jp z,hexHeaderDivider
    cp 3
    jp z,hexTrailerDivider
    ld hl,(hexOffset)
    call hexGrid
    ld a,l
    cpl
    ld c,a
    ld b,0
    inc bc
    ld hl,(hexOffset)
    call hexChecksum
    ld d,b
    ld e,c
    call hexBlockColumn
hexBlockSlot:
    push de
    call hexRowCell
    pop de
    jp hexWord
hexBlockColumn:
    ld a,(hexSumCol)
    or a
    ret nz
    ld a,(hexTextCol)
    ret
hexChecksum:
    call hexSpan
    jp c,hexSplitSum
    jp hexSumSpan
; A block spanning the gap is summed by logical address. Monitor-2 adds
; the low and high bytes of the prefix sum for B, then the last byte for C.
hexSplitSum:
    ld b,d
    ld c,e
    dec bc                     ; a split span contains at least two bytes
    ld de,0
hexSumPrefix:
    call gapRead
    add a,e
    ld e,a
    jp nc,hexPrefixNext
    inc d
hexPrefixNext:
    inc hl
    dec bc
    ld a,b
    or c
    jp nz,hexSumPrefix
    call gapRead
    add a,e
    ld c,a
    ld a,d
    add a,e
    ld b,a
    ret

hexLoadAddress:
    call hexSection
    ld a,e
    or a
    ex de,hl
    ret z
    ld hl,(hexLoad)
    add hl,de
    ex de,hl
    ret
hexDrawAddress:
    ld hl,(hexCaret)
    ex de,hl
    ld hl,(hexPrintedCaret)
    call compareHLDE
    ret z
    ex de,hl
    ld (hexPrintedCaret),hl
    call hexLoadAddress
    ld hl,hexRow
    call hexWord
    COLOR COLOR_STATUSBAR
    call enableInverse
    ld a,(v_charStep)
    ld l,90
    cp 2
    jp z,hexAddressX
    ld l,60                 ; 6x8 caret address fills the last four ruler cells.
    cp 4
    jp nz,hexAddressX
    ld l,0
hexAddressX:
    push af
    call editorCursorX
    pop af
    cp 4
    ld a,8
    jp nz,hexAddressY
    add a,10
hexAddressY:
    ld hl,hexHeaderY
    add a,(hl)
    ld (bios_vars.cursorY),a
    ld hl,hexRow
    ld bc,4
    call hexPrintAt
    jp hexRulerDone

hexPlaceCursor:
    ld hl,(hexTop)
    ex de,hl
    ld hl,(hexCaretRow)
    call bios_sub_hl_de
    ld a,l
    call hexScreenY
    ld (bios_vars.cursorY),a
hexPlaceByteX:
    ld hl,(hexCaret)
    call hexGrid
    ld a,(hexByteMask)
    and l
    call hexColumn
    ld l,a
    call editorCursorX
hexPlaceNibble:
    ld hl,hexNibble
    ld b,a
    ld a,(hl)
    or a
    ld a,b
    jp z,hexStoreCaretX
    ld a,(v_charStep)
    add a,b
hexStoreCaretX:
    ld (bios_vars.cursorX),a
    ret

; The same byte-column table positions digits and ruler labels. Place each
; label over the low nibble, leaving a space after the word Address.
hexDrawRuler:
    COLOR COLOR_STATUSBAR
    call enableInverse
    ld a,(hexDamageCol)
    or a
    jp nz,hexHeaderEncoding
    call hexBlankRow
    ld hl,hexAddressLabel
    ld de,hexAddressLabel+7
    ld a,(v_charStep)
    cp 3
    jp nz,hexRulerAddress
    ld de,hexAddressLabel+4
hexRulerAddress:
    ld bc,hexRow
    call bios_memcpy_bc_hl
    ld c,0
    call hexRulerDigits
    ld a,(hexSumCol)
    or a
    jp z,hexHeaderPrint
    call hexRowCell
    ld (hl),'C'
    inc hl
    ld (hl),'S'
hexHeaderPrint:
    xor a
    ld (bios_vars.cursorY+1),a
    ld a,(hexHeaderY)
    add a,8
    ld (bios_vars.cursorY),a
    ld a,(hexLineLen)
    ld c,a
    ld b,0
    ld hl,hexRow
    call hexPrintAt
    ld a,(hexAcross)
    cp 8
    jp nz,hexHeaderEncoding
    call hexBlankRow
    ld c,8
    call hexRulerDigits
    xor a
    ld (bios_vars.cursorY+1),a
    ld a,(hexHeaderY)
    add a,18
    ld (bios_vars.cursorY),a
    ld hl,hexRow
    ld bc,30
    call hexPrintAt
hexHeaderEncoding:
    ld a,(hexTextCol)
    or a
    jp z,hexRulerDone
    ld l,a
    ld a,(hexAcross)
    cp 8
    jp nz,hexHeaderX
    ld l,37
hexHeaderX:
    call editorCursorX
hexHeaderCharset:
    ld a,(hexFiltered)
    or a
    ld hl,hexAllLabel
    jp z,hexHeaderLabel
    ld hl,hexKoi8Label
    jp p,hexHeaderLabel
    ld hl,hexKoi7Label
hexHeaderLabel:
    ld a,(hexHeaderY)
    add a,8
    ld (bios_vars.cursorY),a
    ld bc,10
    call hexPrintAt
hexRulerDone:
    call disableInverse
    COLOR COLOR_TEXT
    ret

hexRulerDigits:
    ld a,(hexAcross)
    ld b,a
    ld hl,(hexColumns)
hexRulerDigit:
    ld a,(hl)
    inc hl
    push hl
    inc a
    call hexRowCell
    ld a,c
    call hexDigit
    pop hl
    inc c
    dec b
    jp nz,hexRulerDigit
    ret
hexAddressLabel: DB 'Address'
HEX_ROW_BYTES = 86
hexLayout4: DB HEX_ROW_BYTES,63,70,0
    DB 7,10,13,16,21,24,27,30,35,38,41,44,49,52,55,58
    DB 5,19,33,47,61,68,0
hexLayout8: DB 47,32,39,1
    DB 7,10,13,16,19,22,25,28
    DB 5,30,37,0
; Both rows fit before the right-hand scrollbar. Keep all 16 KOI cells
; by starting the first pair immediately after the left separator.
HEX_CS6_COL = 57
hexLayoutCS6: DB 62,HEX_CS6_COL,0,1
    DB 7,10,13,16,19,22,25,28,31,34,37,40,43,46,49,52
    DB 5,55,0
hexLayoutKOI6: DB 62,0,46,0
    DB 6,8,11,13,16,18,21,23,26,28,31,33,36,38,41,43
    DB 5,0
    ASSERT 62*6 <= 376
hexUniformBytes: DB 0
hexFilters6: DB 1,255,1,0
hexAllLabel: DB 'All glyphs'
hexKoi8Label: DB 'KOI8-R    '
hexKoi7Label: DB 'KOI7-R    '
hexEntry: DW 0
hexExpectedCS: DW 0
    IF !EDIT_BANKED
; Text reconstructs the entire visible width table when leaving HEX.
editorPageWidths:
    ENDIF
hexWorkspace:
hexFirstY: DB 0
hexDamageCol: DB 0
hexOldFilter: DB 0
hexPatchFirst: DB 0
hexPatchEnd: DB 0
hexFinalSummary: DW 0
hexPrintedCaret: DW 0
hexDisplayRow: DW 0
hexSummary: DB 0
hexBlockRows: DB 0
hexByteMask: DB 0
hexBase: DW 0
hexLoad: DW 0
hexViewType: DB 0
hexHeaderY: DB 0
hexSize: DW 0
hexAcross: DB 0
hexLineLen: DB 0
hexSumCol: DB 0
hexTextCol: DB 0
hexColumns: DW 0
    IF EDIT_HEX_PSEUDO
hexRules: DW 0
    ENDIF
hexShiftRows: DB 0
hexMarkDirty: DB 0
hexGoal: DB 0
hexCaretRow: DW 0
hexCachedLine: DW 0
hexLineBytes: DW 0
hexLineCount: DB 0
hexOffset: DW 0
hexPaintedTop: DW 0
hexDrawLimit: DB 0
hexDirtyScreen: DB 0
hexReformatEnd: DB 0
hexY: DB 0
hexCount: DB 0
hexSlotsLeft: DB 0
hexMarkFirst: DB 0
hexMarkEnd: DB 0
    IF EDIT_BANKED
; Filename input and HEX redraw do not overlap. Help and Save return to
; hexRepaint, which invalidates the cached pointer into hexScratch.
hexRow = inputBuf
hexScratch = hexRow+HEX_ROW_BYTES
    ASSERT HEX_ROW_BYTES+16 <= MAX_PATH
    ELSE
hexRow = editorRowBuffer
hexScratch: DS 16,0
    ASSERT $-hexWorkspace <= EDITOR_ROWS*3
    DS EDITOR_ROWS*3-($-hexWorkspace),0
    ENDIF
hexBar: UI_SCROLLBAR 376,EDITOR_TEXT_TOP,EDITOR_BAR_Y-EDITOR_TEXT_TOP,8,3,1,EDITOR_ROWS,0

    INCLUDE "hex-edit.inc"
hexContainerEntryLabel: DB 'Entry'
hexContainerEndLabel: DB 'End  '
hexContainerTitle: DB 'MX2  Load '
hexContainerLoad: DB '0000  '
hexContainerEntry: DB 'Entry'
    DB ' '
hexContainerEntryValue: DB '0000  CS '
hexContainerSum: DB '0000'
hexContainerWarn: DB ' '
hexContainerEnd:
; HL=file offset -> HL=relative offset, DE=section start (0 or header length).
; BC is preserved. Only the body starts at the load address.
hexSection:
    ld de,0
    ld a,(hexBase)
    or a
    ret z
    ld e,a
    call bios_sub_hl_de
    ret nc
    add hl,de
    ld e,0
    ret

; HL=file offset -> remaining bytes in the same section.
hexRemaining:
    push hl
    call hexSection
    ld a,e
    or a
    ld hl,(hexSize)
    jp nz,hexRemainingEnd
    ld a,(hexBase)
    or a
    jp z,hexRemainingEnd
    ld l,a
    ld h,0
hexRemainingEnd:
    pop de
    jp bios_sub_hl_de

; Avoid the unused columns at the end of a short RKS header.
hexBoundSection:
    ld a,e
    or a
    ret nz
    ld a,(hexBase)
    or a
    ret z
    dec a
    cp l
    ret nc
    ld l,a
    ret

hexHeaderGeometry:
    xor a
    ld (hexHeaderRows),a
    ld a,(hexBase)
    or a
    ret z
    dec a
    ld l,a
    ld h,0
    call hexDivideRow
    inc l
    inc l
    ld a,l
    ld (hexHeaderRows),a
    ret

hexHeaderDivider:
    call hexDividerTarget
    ld hl,hexHeaderBlockLabel
    ld de,hexHeaderBlockLabel+6
    call bios_memcpy_bc_hl
    ld a,(hexViewType)
    cp 1
    ret nz
    ld hl,(v_textBeginLo)
    ld de,IMAGE_HEADER_CHECKSUM
    add hl,de
    call gapRead
    ld e,a
    inc hl
    call gapRead
    ld d,a
    call hexBlockColumn
    jp hexBlockSlot

hexHeaderRows: DB 0
hexHeaderBlockLabel: DB 'Header'
; The load address contributes only the low nibble to the body's grid.
hexGrid:
    call hexSection
    ld a,e
    or a
    ret z
    ld a,(hexAlign)
    add a,l
    ld l,a
    ret nc
    inc h
    ret
hexFirstHalf:
    call hexGrid
    ld a,e
    or a
    jp z,hexFirstHalfBit
    ld a,(hexAlign)
    cp l
    jp nz,hexFirstHalfBit
    ld a,h
    or a
    ret z
hexFirstHalfBit:
    ld a,l
    and 8
    ret
hexRowColumns:
    ld a,(hexLead)
    ld e,a
    ld d,0
    ld hl,(hexColumns)
    add hl,de
    ret
hexTextColumnStart:
    ld a,(hexTextCol)
    or a
    ret z
hexAddLead:
    push hl
    ld hl,hexLead
    add a,(hl)
    pop hl
    ret
hexTrailerDivider:
    call hexDividerTarget
    ld hl,hexTrailerLabel
    ld de,hexTrailerLabel+6
    call bios_memcpy_bc_hl
    ld hl,(hexExpectedCS)
    ex de,hl
    call hexBlockColumn
    jp hexBlockSlot
hexDividerTarget:
    HEX_ROW_TARGET
    ld b,h
    ld c,l
    ret
hexTrailerLabel: DB 'RKS CS'
hexTrailerRow: DW 0
hexLead: DB 0
hexAlign: DB 0
