; Shifted editing commands share the document primitives with text mode.
hexBlockKeys:
    E_KEY 'F1','Copy'
    E_KEY 'F2','Zero'
    E_KEY 'F3','Fill'
    E_KEY 'F4','Ovr'
    UI_TOOLBAR_FIELD 17,080h,' '
    UI_TOOLBAR_FIELD 3,0,'F10'
    UI_TOOLBAR_FIELD 4,080h,'Exit'
    DB 0

; A=F1..F4. Return the original key unless Shift selects a block command.
hexCheckBlockKey:
    push af
    call testShiftKey
    jp nz,hexDispatchBlock
    pop af
    ret
hexDispatchBlock:
    pop af
    pop hl                    ; the command returns through hexUpdate
    or a
    jp z,hexCopyBlock
    dec a
    jp z,hexInsertZeros
    dec a
    jp z,hexFillWord
    jp hexOverwriteBlock

hexCopyBlock:
    call hexRequireRaw
    call hexBlockTarget
    jp c,hexNoSelection
    ex de,hl
    ld hl,(v_selectionBegin)
    call compareHLDE
    jp nc,hexCopyReady
    ld hl,(v_selectionEnd)
    call compareHLDE
    jp c,hexCopyReady
    jp nz,hexCopyOverlap
hexCopyReady:
    call hexCheckInsert
    call editorCopyBlockBytes
    jp hexBlockInserted

hexInsertZeros:
    call hexRequireRaw
    ld de,hexZeroTitle
    call hexReadBlockWord
hexInsertZeroCount:
    ld a,h
    or l
    jp z,hexResumeModal
    ld (editorBlockLength),hl
    call hexAbsoluteCaret
    ld (editorBlockTarget),hl
    call hexCheckInsert
    call editorGrowBlock
    ld bc,0
    call editorFillBlockBytes
hexBlockInserted:
    push hl
    call hexRawLength
    pop hl
hexBlockAdvance:
    ex de,hl
    ld hl,(v_textBeginLo)
    ex de,hl
    call nativeMath.sub_hl_de
    ld (hexCaret),hl
    jp hexBlockChanged

hexOverwriteBlock:
    call hexBlockTarget
    jp c,hexNoSelection
    ex de,hl
    ld hl,(editorBlockLength)
    add hl,de
    push hl
    ld hl,(v_textBeginLo)
    ex de,hl
    ld hl,(hexSize)
    add hl,de
    pop de
    call compareHLDE
    jp c,hexOverwriteOutside
    call editorFreezeMark
    call editorOverwriteBlockBytes
    jp hexBlockAdvance

hexFillWord:
    call hexSelectionLength
    jp c,hexNoSelection
    ld de,hexWordTitle
    call hexReadBlockWord
hexFillValue:
    push hl
    call editorFreezeMark
    call commitLineEdit
    ld hl,(v_selectionEnd)
    call gapMove
    ld hl,(v_selectionEnd)
    ex de,hl
    ld hl,(v_selectionBegin)
    pop bc
    call editorFillBlockBytes
    call editorSetDirty
hexBlockChanged:
    ld a,1
    ld (hexChangedBreak),a
    xor a
    ld (hexNibble),a
    ld a,(hexViewType)
    or a
    jp z,hexBlockRefresh
    call hexTryContainer
    jp c,hexContainerInvalid
    call hexDrawContainer
hexBlockRefresh:
    call hexViewportEnd
    ld (hexReformatEnd),a
    ld a,(hexDirtyScreen)
    push af
    call hexPrepareGeometry
    pop af
    cp 1                       ; MAIN may need to rebuild the modal underlay
    jp z,hexUpdate
    ld a,4
    ld (hexDirtyScreen),a
    jp hexUpdate

hexBlockTarget:
    call hexSelectionLength
    ret c
    ld (editorBlockLength),hl
    call hexAbsoluteCaret
    ld (editorBlockTarget),hl
    ret
hexSelectionLength:
    ld hl,(v_selectionBegin)
    ld a,h
    or l
    jp z,hexEmptySelection
    ex de,hl
    ld hl,(v_selectionEnd)
    call nativeMath.sub_hl_de
    ret c
    ld a,h
    or l
    ret nz
hexEmptySelection:
    scf
    ret
hexRequireRaw:
    ld a,(hexViewType)
    or a
    ret z
    pop hl
    ld hl,hexRawTitle
    jp hexBlockError
hexCheckInsert:
    call gapCapacity
    push hl
    ld hl,(editorBlockLength)
    ex de,hl
    pop hl
    call compareHLDE
    jp nc,editorFreezeMark
    pop hl
    ld hl,editorMemoryMessage
    jp hexBlockError
hexNoSelection:
    ld hl,hexSelectTitle
    jp hexBlockError
hexCopyOverlap:
    ld hl,hexOverlapTitle
    jp hexBlockError
hexOverwriteOutside:
    ld hl,hexOverwriteTitle
hexBlockError:
    call editorErrorModal
    call editorCloseModal
    jp hexResumeModal

; Use the shared input dialog. Return HL on successful input.
hexReadBlockWord:
    ld hl,editorFileDescriptor
    call editorOpenInputModal
    ld hl,inputBuf
    ld d,h
    ld e,l
    ld a,(editorInputWidth)
    ld c,a
    ld b,1
    call lineEdit
    cp 13
    jp nz,hexCancelBlockInput
    call hexParseBlockWord
    push af
    push hl
    call editorCloseModal
    pop hl
    pop af
    ret nc
    pop hl
    ld hl,hexBadWordTitle
    jp hexBlockError
hexCancelBlockInput:
    call editorCloseModal
    pop hl
    jp hexResumeModal

hexParseBlockWord:
    ld hl,inputBuf
    ld de,0
    ld b,4
hexParseBlockDigit:
    ld a,(hl)
    or a
    jp z,hexParseBlockEnd
    call hexDecodeDigit
    ret c
    inc hl
    ex de,hl
    add hl,hl
    add hl,hl
    add hl,hl
    add hl,hl
    or l
    ld l,a
    ex de,hl
    dec b
    jp nz,hexParseBlockDigit
    ld a,(hl)
    or a
    jp nz,hexInvalidDigit
hexParseBlockEnd:
    ld a,b
    cp 4
    jp z,hexInvalidDigit
    ex de,hl
    or a
    ret

hexDecodeDigit:
    sub '0'
    ret c
    cp 10
    jp c,hexDecodedDigit
    sub 'A'-'0'
    and 0DFh
    cp 6
    jp nc,hexInvalidDigit
    add a,10
hexDecodedDigit:
    or a
    ret
hexInvalidDigit:
    scf
    ret

hexZeroTitle:       DB 'Zero byte count (hex)',0
hexWordTitle:       DB 'Fill word (hex)',0
hexRawTitle:        DB 'F7: switch to raw HEX.',0
hexSelectTitle:     DB 'Mark a block with F4.',0
hexOverlapTitle:    DB 'Caret is inside the block.',0
hexOverwriteTitle:  DB 'Block exceeds end of data.',0
hexBadWordTitle:    DB 'Enter 1-4 hex digits.',0
