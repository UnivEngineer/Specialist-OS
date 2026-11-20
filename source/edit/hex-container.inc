; The container view changes only the visible span and address origin.
hexRawView:
    ld hl,0
    ld (hexBase),hl
    ld (hexLoad),hl
    xor a
    ld (hexViewType),a
    ld (hexHeaderY),a
    ld (hexHeaderRows),a
hexRawLength:
    ld hl,(v_textBeginLo)
    ex de,hl
    ld hl,(v_textEnd)
    call bios_sub_hl_de
    ld (hexSize),hl
    ret

hexContainerKey:
    ld a,(hexViewType)
    or a
    jp z,hexContainerOpen
    call hexRawView
    jp hexContainerGeometry
hexContainerOpen:
    call hexTryContainer
    jp c,hexContainerInvalid
    jp hexContainerGeometry
hexTryContainer:
    call hexParseContainer
    ret c
    ld a,10
    ld (hexHeaderY),a
    ret
hexRecalcContainer:
    ld a,(hexViewType)
    or a
    jp z,hexWait
    call hexParseContainer
    jp c,hexContainerInvalid
    ld hl,(hexSize)
    ld b,h
    ld c,l
    ld hl,(hexBase)
    call hexChecksum
    ld h,b
    ld l,c
    ld (hexExpectedCS),hl
    ld d,b
    ld e,c
    ld a,(hexViewType)
    cp 2
    ld hl,(hexSize)
    jp z,hexWriteFinalSum
    ld hl,12
    call hexWriteSum
    ld hl,0
    ld bc,IMAGE_HEADER_CHECKSUM
    call hexChecksum
    ld d,b
    ld e,c
    ld hl,IMAGE_HEADER_CHECKSUM
hexWriteFinalSum:
    call hexWriteSum
    ld a,' '
    ld (hexContainerWarn),a
    call hexDrawContainer
    ld a,(hexViewType)
    cp 2
    jp nz,hexUpdate
    ld hl,(hexTrailerRow)
    call hexPaintEditedRow
    jp hexUpdate
hexWriteSum:
    push hl
    push de
    ex de,hl
    ld hl,(v_textBeginLo)
    add hl,de
    pop de
    ld c,e
    push de
    call hexWriteByte
    pop de
    inc hl
    ld c,d
    call hexWriteByte
    pop hl
    jp hexPatchWrittenSum
hexContainerGeometry:
    call hexViewportEnd
    ld (hexReformatEnd),a
    xor a
    ld (hexNibble),a
    ld hl,(hexCaret)
    call hexClamp
    ld (hexCaret),hl
    ld hl,0
    ld (hexTop),hl
    call hexPrepareGeometry
    ld a,3
    ld (hexDirtyScreen),a
    jp hexUpdate
hexContainerInvalid:
    ld a,(hexViewType)
    push af
    call hexRawView
    ld hl,hexUnsupported
    call editorInfoModal
    call editorCloseModal
    pop af
    or a
    jp z,hexResumeModal
    jp hexContainerGeometry

hexParseContainer:
    call hexRawLength
    ld de,7
    call compareHLDE
    jp c,hexInvalidHeader
    ld hl,(v_textBeginLo)
    ld de,hexRow
    ld b,6
    call hexReadHeader
    ld hl,hexRow
    ld de,hexMagic
    ld b,5
hexCheckMagic:
    ld a,(de)
    cp (hl)
    jp nz,hexParseRKS
    inc hl
    inc de
    dec b
    jp nz,hexCheckMagic
    ld a,(hl)
    cp IMAGE_FLAG_MAIN
    jp z,hexMXHeader
    cp IMAGE_FLAG_BANK+1
    jp nc,hexInvalidHeader
hexMXHeader:
    ld hl,(hexSize)
    ld de,IMAGE_HEADER_SIZE+1
    call compareHLDE
    jp c,hexInvalidHeader
    ld hl,(v_textBeginLo)
    ld de,hexRow
    ld b,IMAGE_HEADER_SIZE
    call hexReadHeader
    ld a,(hexRow+IMAGE_HEADER_BYTES)
    cp IMAGE_HEADER_SIZE
    jp nz,hexInvalidHeader
    ld a,(hexRow+29)
    or a
    jp nz,hexInvalidHeader
    ld hl,(hexRow+6)
    ld (hexLoad),hl
    ld hl,(hexRow+8)
    ld (hexEntry),hl
    ld hl,(hexRow+12)
    ld (hexExpectedCS),hl
    ld hl,(hexRow+10)
    ld de,IMAGE_HEADER_SIZE
    add hl,de
    jp c,hexInvalidHeader
    ex de,hl
    ld hl,(hexSize)
    call compareHLDE
    jp nz,hexInvalidHeader
    ld hl,(hexRow+10)
    ex de,hl
    ld hl,(hexLoad)
    dec de
    add hl,de
    jp c,hexInvalidHeader
    ex de,hl
    ld hl,(hexEntry)
    call compareHLDE
    jp z,hexMXEntryUpper
    jp nc,hexInvalidHeader
hexMXEntryUpper:
    ld hl,(hexEntry)
    ex de,hl
    ld hl,(hexLoad)
    call compareHLDE
    jp z,hexMXEntryLower
    jp nc,hexInvalidHeader
hexMXEntryLower:
    ld hl,(hexSize)
    ld de,IMAGE_HEADER_SIZE
    ld a,1
    jp hexInstallSpan

hexParseRKS:
    call editorHexFileKind
    cp 2
    jp nz,hexInvalidHeader
    ld hl,(hexRow)
    ld (hexLoad),hl
    ex de,hl
    ld hl,(hexRow+2)
    ld (hexEntry),hl
    call bios_sub_hl_de
    jp c,hexInvalidHeader
    ld de,7
    add hl,de
    jp c,hexInvalidHeader
    ex de,hl
    ld hl,(hexSize)
    call compareHLDE
    jp nz,hexInvalidHeader
    ld hl,(v_textEnd)
    dec hl
    dec hl
    ld de,hexExpectedCS
    ld b,2
    call hexReadHeader
    ld hl,(hexSize)
    dec hl
    dec hl
    ld de,4
    ld a,2
    jp hexInstallSpan

; Detect an executable suffix after a successful document open. RKS also
; needs the suffix when F7 is pressed because the format has no signature.
editorDetectHex:
    call editorHexFileKind
    ld (editorPendingHex),a
    ret
editorHexFileKind:
    ld hl,FILE_NAME_HISTORY
    ld de,0
hexExtensionScan:
    ld a,(hl)
    or a
    jp z,hexCheckExtension
    inc hl
    cp '.'
    jp nz,hexExtensionScan
    ld d,h
    ld e,l
    jp hexExtensionScan
hexCheckExtension:
    ld a,d
    or e
    ret z
    ex de,hl
    push hl
    ld bc,3
    add hl,bc
    call compareHLDE
    pop hl
    jp nz,hexUnknownExtension
    ld de,hexExtensions
hexNextExtension:
    ld a,(de)
    or a
    ret z
    push hl
    ld b,3
hexMatchExtension:
    ld a,(hl)
    and 0DFh
    ex de,hl
    cp (hl)
    ex de,hl
    jp nz,hexSkipExtension
    inc hl
    inc de
    dec b
    jp nz,hexMatchExtension
    pop hl
    ld a,(de)
    ret
hexSkipExtension:
    inc de
    dec b
    jp nz,hexSkipExtension
    inc de
    pop hl
    jp hexNextExtension
hexUnknownExtension:
    xor a
    ret
hexExtensions:
    DB 'EXE',1,'COM',1,'DLL',1,'FNT',1,'RKS',2,0
hexInstallSpan:
    ld (hexViewType),a
    ld (hexSize),hl
    ex de,hl
    ld (hexBase),hl
    ld a,'?'
    ld (hexContainerWarn),a
    or a
    ret
hexReadHeader:
    call gapRead
    ld (de),a
    inc hl
    inc de
    dec b
    jp nz,hexReadHeader
    ret
hexInvalidHeader:
    scf
    ret

hexDrawContainer:
    ld a,(hexViewType)
    or a
    ret z
    ld hl,hexContainerEntryLabel
    dec a
    ld de,'M'+'X'*256
    jp z,hexContainerLabel
    ld hl,hexContainerEndLabel
    ld de,'R'+'K'*256
hexContainerLabel:
    ex de,hl
    ld (hexContainerTitle),hl
    ex de,hl
    ld a,(hexViewType)
    cp 1
    ld a,'2'
    jp z,hexContainerName
    ld a,'S'
hexContainerName:
    ld (hexContainerTitle+2),a
    push hl
    ld de,5
    add hl,de
    ex de,hl
    pop hl
    ld bc,hexContainerEntry
    call bios_memcpy_bc_hl
    ld hl,(hexLoad)
    ex de,hl
    ld hl,hexContainerLoad
    call hexWord
    ld hl,(hexEntry)
    ex de,hl
    ld hl,hexContainerEntryValue
    call hexWord
    ld hl,(hexExpectedCS)
    ex de,hl
    ld hl,hexContainerSum
    call hexWord
    COLOR COLOR_STATUSBAR
    call enableInverse
    ld hl,0008h
    ld (bios_vars.cursorY),hl
    ld hl,hexContainerTitle
    ld bc,hexContainerEnd-hexContainerTitle
    call hexPrintAt
    jp hexRulerDone
hexMagic:
    MXOS_IMAGE_SIGNATURE
