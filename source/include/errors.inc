; Common DOS error strings from A:\ERRORS.DAT.
; A=error code, HL=NUL-terminated local text, BC/DE preserved.
; The result lasts until the next lookup. Uses the local file-io handle.
; Codes outside the index or an unavailable database show a local hex code.
DOS_ERROR_RECORDS = 14
DOS_ERROR_INDEX_BYTES = DOS_ERROR_RECORDS*3
DOS_ERROR_TEXT_MAX = 31
dosErrorText:
    push bc
    push de
    ld (dosTextCode),a
    ld e,0
    MXOS_CALL MXOS_DOS_FILE_GET_SET_DRIVE
    ld (dosTextDrive),a
    ld a,(dosTextCode)
    cp DOS_ERROR_RECORDS
    jp nc,dosTextFallback
    call flClear
    ld hl,dosTextPath
    ld (flHandle+FH_BUFFER),hl
    ld a,FILE_MODE_READ
    ld (flHandle+FH_MODE),a
    ld a,FILE_OP_OPEN_PATH
    call flCall
    jp c,dosTextFallback
    ld a,(dosTextCode)
    ld c,a
    add a,a
    add a,c
    ld l,a
    ld h,0
    ld bc,3
    call dosTextReadAt
    jp c,dosTextClose
    ld hl,(dosTextBuffer)
    ld de,DOS_ERROR_INDEX_BYTES
    call bios_cmp_hl_de
    jp c,dosTextBad
    ld a,(dosTextBuffer+2)
    or a
    jp z,dosTextBad
    cp DOS_ERROR_TEXT_MAX+1
    jp nc,dosTextBad
    ld c,a
    ld b,0
    call dosTextReadAt
    jp c,dosTextClose
    ld hl,(flHandle+FH_COUNT)
    ld de,dosTextBuffer
    add hl,de
    ld (hl),0
    or a
    jp dosTextClose
dosTextBad:
    scf
dosTextClose:
    call flClose
    ld hl,dosTextBuffer
    jp nc,dosTextReturn
dosTextFallback:
    ld hl,dosTextHex
    ld a,(dosTextCode)
    rrca
    rrca
    rrca
    rrca
    call dosTextHexDigit
    ld (hl),a
    inc hl
    ld a,(dosTextCode)
    call dosTextHexDigit
    ld (hl),a
    ld hl,dosTextUnavailable
dosTextReturn:
    push hl
    ld a,(dosTextDrive)
    MXOS_CALL MXOS_DOS_FILE_SELECT_DRIVE
    pop hl
    pop de
    pop bc
    ret
dosTextHexDigit:
    and 0Fh
    add a,'0'
    cp '9'+1
    ret c
    add a,'A'-'9'-1
    ret

; HL=absolute offset, BC=expected byte count, near destination dosTextBuffer.
dosTextReadAt:
    push bc
    ld (flHandle+FH_SEEK),hl
    ld hl,0
    ld (flHandle+FH_SEEK+2),hl
    ld a,FILE_OP_SEEK
    call flCall
    pop bc
    ret c
    ld hl,dosTextBuffer
    ld (flHandle+FH_BUFFER),hl
    ld h,b
    ld l,c
    ld (flHandle+FH_COUNT),hl
    push hl
    ld a,FILE_OP_READ
    call flCall
    pop de
    ret c
    ld hl,(flHandle+FH_COUNT)
    call bios_cmp_hl_de
    ret z
    scf
    ret

dosTextPath: DB 'A:',05Ch,'ERRORS.DAT',0
dosTextUnavailable: DB 'DOS error '
dosTextHex: DB '00h',0
dosTextCode: DB 0
dosTextDrive: DB 0
dosTextBuffer: DS DOS_ERROR_TEXT_MAX+1,0
