; A digit immediately replaces one nibble. Navigation starts a new byte.
hexInputDigit:
    call hexDecodeDigit
    jp c,hexWait
    ld b,a
    ld hl,(hexCaret)
    ex de,hl
    ld hl,(hexSize)
    call compareHLDE
    jp c,hexWait
    jp z,hexWait
    call hexAbsoluteCaret
    call testPositionSelected
    jp c,hexWait
    call gapRead
    ld e,a
    ld a,(hexNibble)
    or a
    ld a,e
    jp nz,hexReplaceLow
    and 0Fh
    ld c,a
    ld a,b
    add a,a
    add a,a
    add a,a
    add a,a
    jp hexReplaceDigit
hexReplaceLow:
    and 0F0h
    ld c,a
    ld a,b
hexReplaceDigit:
    or c
    ld c,a
    cp e
    jp z,hexAdvanceDigit
    call hexWriteByte
    call hexContainerEdited
    call hexPaintByteChange
hexAdvanceDigit:
    jp hexRight

hexWriteByte:
    call gapRead
    cp c
    ret z
    call hexNoteBreak
    ld a,c
    call hexNoteBreak
    push hl
    call gapWrite
    call commitLineEdit
    ld hl,0FFFFh
    ld (hexCachedLine),hl
    call editorSetDirty
    pop hl
    ret

; Paint one digit and its text glyph, or both digits when F8 changed a byte.
hexPatchByte:
    ld hl,(hexCaret)
    call hexFileRow
    call hexLocateRow
    ret c
    call hexPlaceByteX
    call hexAbsoluteCaret
    call testPositionSelected
    call c,enableInverse
    call gapRead
    push af
    ld hl,hexRow
    call hexByte
    ld hl,hexRow
    ld bc,2
    ld a,(hexPatchWord)
    or a
    jp nz,hexPatchDigits
    dec c
    ld a,(hexNibble)
    or a
    jp z,hexPatchDigits
    inc hl
hexPatchDigits:
    call hexPrintAt
    pop af
    ld c,a
    ld a,(hexTextCol)
    or a
    jp z,hexPatchNoText
    ld hl,(hexCaret)
    call hexGrid
    ld a,(hexByteMask)
    and l
    ld hl,hexTextCol
    add a,(hl)
    ld l,a
    push bc
    call editorCursorX
    pop bc
    ld a,(hexFiltered)
    call hexGlyph
    ld (hexRow),a
    call hexSetRowY
    ld hl,hexRow
    ld bc,1
    call hexPrintAt
hexPatchNoText:
    jp disableInverse

; Locate the checksum of the edited logical row and its block. For 8x8,
; use the populated second half when it exists.
hexPaintByteChange:
    ld a,(hexDirtyScreen)
    or a
    ret nz
    call hexPatchByte
    ld hl,(hexCaret)
    call hexGrid
    ld a,l
    and 0F0h
    ld l,a
    push de
    push hl
    call hexGridFile
    call hexDataRow
    call hexPaintEditedRow
    pop hl
    pop de
    ld a,(hexAcross)
    cp 8
    jp nz,hexPaintBlockChange
    ld a,l
    or 8
    ld l,a
    ld c,l
    call hexBoundSection
    ld a,l
    cp c
    jp nz,hexPaintBlockChange
    call hexGridFile
    push hl
    call hexRemaining
    jp c,hexPaintHalfOutside
    ld a,h
    or l
    pop hl
    jp z,hexPaintBlockChange
    call hexDataRow
    call hexPaintEditedRow
    jp hexPaintBlockChange
hexPaintHalfOutside:
    pop hl
hexPaintBlockChange:
    ld hl,(hexCaret)
    call hexGrid
    ld a,e
    or a
    jp nz,hexPaintBodySum
    ld a,(hexHeaderRows)
    or a
    jp z,hexPaintBodySum
    ld l,a
    ld h,0
    dec hl
    jp hexPaintEditedRow
hexPaintBodySum:
    inc h
    ld l,0
    call hexGridFile
    ex de,hl
    ld hl,(hexSize)
    call compareHLDE
    jp nc,hexFullBlockRow
    ld hl,(hexFinalSummary)
    jp hexPaintEditedRow
hexFullBlockRow:
    ex de,hl
    call hexDataRow
    dec hl
hexPaintEditedRow:
    IF EDIT_BANKED
    call hexCacheDrop
    ENDIF
    push hl
    call hexLocateRow
    pop hl
    ret c
    call hexDecodeRow
    ld a,(hexSummary)
    or a
    jp nz,hexPaintEditedBlock
    call hexSetCount
    or a
    ret z
    call hexSelectLine
    call hexTailColumn
    ret nz
    ld a,(hexSumCol)
    jp hexPaintSum
hexPaintEditedBlock:
    ld a,(hexSummary)
    cp 2
    jp nz,hexPaintBlockValue
    ld a,(hexViewType)
    cp 2
    ret z                      ; RKS has no stored header sum.
hexPaintBlockValue:
    call hexFormatBlock
    call hexBlockColumn
hexPaintSum:
    ld c,4
    jp hexPrintSpan

; HL=physical row. CF if outside the viewport.
hexLocateRow:
    push hl
    ld hl,(hexTop)
    ex de,hl
    pop hl
    call nativeMath.sub_hl_de
    ld a,h
    or a
    jp nz,hexRowOutside
    ld a,l
    ld hl,hexBar.visible
    cp (hl)
    jp nc,hexRowOutside
    call hexScreenY
    ld (hexY),a
    ld (mon2_vars.cursorY),a
    or a
    ret
hexRowOutside:
    scf
    ret

hexContainerEdited:
    ld a,(hexViewType)
    or a
    ret z
    ld hl,hexContainerWarn
    ld b,(hl)
    ld (hl),'?'
    ld hl,(hexCaret)
    call hexSection
    ld a,e
    or a
    ld a,b
    jp z,hexEditedHeader
    cp '?'
    ret z
    jp hexDrawContainer
hexEditedHeader:
    ld hl,(hexLoad)
    push hl
    ld hl,(v_textBeginLo)
    ld a,(hexViewType)
    cp 2
    jp z,hexHeaderReadAddresses
    ld de,6
    add hl,de
hexHeaderReadAddresses:
    ld de,hexLoad
    ld b,2
    call hexReadHeader
    ld de,hexEntry
    ld b,2
    call hexReadHeader
    ld a,(hexViewType)
    cp 2
    jp z,hexCompareLoad
    inc hl
    inc hl
    ld de,hexExpectedCS
    ld b,2
    call hexReadHeader
hexCompareLoad:
    pop de
    ld hl,(hexLoad)
    call compareHLDE
    jp nz,hexPrepareGeometry
    jp hexDrawContainer

hexNoteBreak:
    cp 13
    jp z,hexBreakChanged
    cp 10
    ret nz
hexBreakChanged:
    ld (hexChangedBreak),a
    ret

    IF !EDIT_BANKED
    INCLUDE "hex-text-return.inc"
    ENDIF
hexNibble:      DB 0
hexPatchWord:   DB 0

hexPatchWrittenSum:
    ld a,(hexViewType)
    cp 2
    ret z
    push hl
    ld hl,(hexCaret)
    ex (sp),hl
    ld a,(hexNibble)
    push af
    xor a
    ld (hexNibble),a
    inc a
    ld (hexPatchWord),a
    ld (hexCaret),hl
    call hexPaintByteChange
    ld hl,hexCaret
    inc (hl)
    call hexPaintByteChange
    xor a
    ld (hexPatchWord),a
    pop af
    ld (hexNibble),a
    pop hl
    ld (hexCaret),hl
    ret
