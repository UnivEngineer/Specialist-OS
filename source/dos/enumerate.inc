; Shared overlay facade. Never borrow the parser's saved CWD or scratch buffers.
    IFDEF BANKED_DOS_BUILD
enumRun:
    ld (enumOperation),a
    push hl
    push bc
    or a
    jp nz,enumExisting
    ld a,(enumActive)
    or a
    jp nz,enumBusy
    ld hl,enumOverlay
    call overlayEnsure
    jp enumPrepared
enumBusy:
    call overlayBusy
    jp enumPrepared
enumExisting:
    ld a,(enumActive)
    or a
    call z,blockArgument
enumPrepared:
    pop bc
    pop hl
    ret c
    push hl
    push bc
    ld hl,v_directories
    ld de,enumDirectories
    ld bc,16
    call moveBytesForward
    ld hl,(v_directory)
    ld (enumDirectory),hl
    ld a,(v_drive)
    ld (enumDrive),a
    pop bc
    pop hl
    ld a,(enumOperation)
    call DOS_SERVICE_ADDR
    push af
    push hl
    ld hl,enumDirectories
    ld de,v_directories
    ld bc,16
    call moveBytesForward
    ld a,(enumDrive)
    call fileChangeDrive
    ld hl,(enumDirectory)
    ld (v_directory),hl
    pop hl
    pop af
    push af
    ld a,(enumOperation)
    cp ENUM_OPEN
    jp z,enumOpened
    cp ENUM_CLOSE
    jp nz,enumReturned
    xor a
    ld (enumActive),a
    jp enumReturned
enumOpened:
    pop af
    jp c,enumReturnError
    push af
    ld a,1
    ld (enumActive),a
enumReturned:
    pop af
enumReturnError:
    ld (v_dosError),a
    ret
    ELSE
enumRun:
    jp dosUnsupported
    ENDIF
