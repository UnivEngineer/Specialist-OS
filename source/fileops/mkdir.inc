; MKDIR parents: bounded path prefixes, no recursive CPU stack or saved tree.
; An existing directory is success. A file in any component is an error.
foMkdir:
    push hl
    call foContextSave
    pop hl
    ld de,fmPath
    ld b,MAX_PATH
fmCopy:
    ld a,(hl)
    ld (de),a
    inc hl
    inc de
    or a
    jp z,fmStart
    dec b
    jp nz,fmCopy
    call blockArgument
    jp fmDone
fmStart:
    ld hl,fmPath
fmScan:
    ld a,(hl)
    or a
    jp z,fmPrefix
    cp '/'
    jp z,fmPrefix
    cp 05Ch
    jp z,fmPrefix
    inc hl
    jp fmScan
fmPrefix:
    ld (fmCursor),hl
    ld (fmSeparator),a
    ld (hl),0
    ld a,(fmPath)
    or a
    jp z,fmContinue
    call cliCheckCancel
    jp c,fmDone
    call foContextRestore
    ld hl,fmPath
    call directoryPathUnchecked
    jp nc,fmContinue
    ld a,(v_dosError)
    cp DOS_ERR_NOT_FOUND
    jp nz,fmError
    call foContextRestore
    ld hl,fmPath
    call directoryCreateSingle
    jp c,fmDone
fmContinue:
    ld hl,(fmCursor)
    ld a,(fmSeparator)
    ld (hl),a
    or a
    jp z,fmSuccess
    inc hl
    jp fmScan
fmError:
    scf
    jp fmDone
fmSuccess:
    xor a
    ld (v_dosError),a
fmDone:
    push af
    call foContextRestore
    pop af
    ret
fmCursor:       DW 0
fmSeparator:    DB 0
fmPath:         DS MAX_PATH,0
