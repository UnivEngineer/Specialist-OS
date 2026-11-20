; File-backed help viewer. Include after ui-modal.inc and file-io.inc or
; elsewhere in the same image. Client supplies:
; UI_HELP_OPEN (HL=modal descriptor), UI_HELP_KEY (A=key, CF=Ctrl),
; UI_HELP_CLOSE, UI_HELP_COLOR (sets/help palette, preserves BC/DE/HL),
; UI_HELP_BUFFER (128 writable bytes, disjoint from flHandle).
; uiShowHelp: HL=absolute ASCIIZ path. Returns CF/A on failure.
; Format: KOI8-R, LF or CRLF, Tab stops at columns 13 and 26 (zero based).
; Each line is clipped to 28 cells. Requests are synchronous, non-reentrant.
UI_HELP_ROWS = 9
UI_HELP_COLUMNS = 28
UI_HELP_BODY = UI_HELP_ROWS*10
UI_HELP_Y = (256-UI_HELP_BODY-58)/2+26
UI_HELP_BLOCK = 128
UI_HELP_INDEX_LAST = 56
UI_HELP_INDEX_BYTES = (UI_HELP_INDEX_LAST/8+1)*4
UI_HELP_BITMAP = 09900h+UI_HELP_Y-1

uiShowHelp:
    push hl
    call flClear
    pop hl
    ld (flHandle+FH_BUFFER),hl
    ld a,FILE_MODE_READ
    ld (flHandle+FH_MODE),a
    ld a,FILE_OP_OPEN_PATH
    call flCall
    ret c
    xor a
    ld (uiHelpError),a
    ld (uiHelpRemaining),a
    ld hl,0
uiHelpCount:
    push hl
    call uiHelpCheckpoint
    call uiHelpLine
    pop hl
    jp c,uiHelpCountDone
    inc hl
    ld a,h
    and 80h
    jp z,uiHelpCount
    ld a,DOS_ERR_ARGUMENT
    scf
    jp flClose
uiHelpCountDone:
    ld a,(uiHelpError)
    or a
    jp nz,uiHelpCloseError
    ld (uiHelpScroll.total),hl
    ld de,UI_HELP_ROWS
    call bios_sub_hl_de
    jp nc,uiHelpLastReady
    ld hl,0
uiHelpLastReady:
    ld (uiHelpLast),hl
    ld hl,uiHelpDescriptor
    call UI_HELP_OPEN
    call UI_HELP_COLOR
    ld hl,uiHelpTitle
    call uiModalTitle
    ld hl,0FFFFh
    ld (uiHelpStreamLine),hl
    ld hl,0
    jp uiHelpFullPage
uiHelpPage:
    ex de,hl
    ld hl,(uiHelpScroll.position)
    call bios_sub_hl_de         ; old - new
    ld a,h
    or l
    jp z,uiHelpRead
    ex de,hl
    ld (uiHelpScroll.position),hl
    inc de
    ld a,d
    or e
    jp z,uiHelpStepDown
    dec de
    dec de
    ld a,d
    or e
    jp z,uiHelpStepUp
uiHelpFullPage:
    ld (uiHelpScroll.position),hl
    call uiHelpGoto
    jp c,uiHelpCloseModal
    ld bc,UI_HELP_ROWS*256+UI_HELP_Y+7
uiHelpPaintLine:
    push bc
    call uiHelpNextRow          ; EOF leaves a blank row
    pop bc
    push bc
    ld a,c
    call uiHelpPrintRow
    pop bc
    ld a,c
    add a,10
    ld c,a
    dec b
    jp nz,uiHelpPaintLine
    jp uiHelpPaintTrack
uiHelpStepDown:
    ld de,UI_HELP_ROWS-1
    add hl,de
    call uiHelpReadEdge
    jp c,uiHelpCloseModal
    ld hl,UI_HELP_BITMAP+10
    ld bc,UI_HELP_BITMAP
    ld a,UI_HELP_Y+7+(UI_HELP_ROWS-1)*10
    jp uiHelpCopy
uiHelpStepUp:
    call uiHelpReadEdge
    jp c,uiHelpCloseModal
    ld hl,UI_HELP_BITMAP
    ld bc,UI_HELP_BITMAP+10
    ld a,UI_HELP_Y+7
uiHelpCopy:
    push af
    call UI_HELP_COLOR
    ld de,UI_HELP_COLUMNS*256+10*(UI_HELP_ROWS-1)
    call uiCopyTextRect
    pop af
    call uiHelpPrintRow
uiHelpPaintTrack:
    ld a,(uiHelpError)
    or a
    jp nz,uiHelpDrawError
    ld hl,uiHelpScroll
    call uiDrawScrollbar
uiHelpRead:
    IFDEF UI_HELP_STEPPED
    xor a
    ret
    ELSE
    call UI_HELP_KEY
    ENDIF
uiHelpKey:
    jp c,uiHelpControl
    cp 13
    jp z,uiHelpCloseOk
    cp 27
    jp z,uiHelpCloseOk
    ld de,0FFFFh
    cp 19h
    jp z,uiHelpMove
    ld de,1
    cp 1Ah
    jp z,uiHelpMove
    jp uiHelpEnds
uiHelpControl:
    ld de,-UI_HELP_ROWS & 0FFFFh
    cp 19h
    jp z,uiHelpMove
    ld de,UI_HELP_ROWS
    cp 1Ah
    jp z,uiHelpMove
uiHelpEnds:
    ld hl,0
    cp 0Ch
    jp z,uiHelpPage
    ld hl,(uiHelpLast)
    cp 0Ah
    jp z,uiHelpPage
    jp uiHelpRead
uiHelpMove:
    ld hl,(uiHelpScroll.position)
    add hl,de
    ld a,h
    or a
    jp p,uiHelpCheckLast
    ld hl,0
uiHelpCheckLast:
    ex de,hl
    ld hl,(uiHelpLast)
    ex de,hl
    call bios_cmp_hl_de
    jp c,uiHelpPage
    ex de,hl
    jp uiHelpPage
uiHelpDrawError:
    scf
    jp uiHelpCloseModal
uiHelpCloseOk:
    xor a
uiHelpCloseModal:
    call flClose
    push af
    call UI_HELP_CLOSE
    pop af
    IFDEF UI_HELP_STEPPED
    ret c
    ld a,1
    or a
    ENDIF
    ret
uiHelpCloseError:
    scf
    jp flClose

uiHelpPrintRow:
    ld (bios_vars.cursorY),a
    ld a,36
    ld (bios_vars.cursorX),a
    call UI_HELP_COLOR
    ld hl,uiHelpRow
    ld bc,UI_HELP_COLUMNS
    jp bios_printStringRawLim

uiHelpReadEdge:
    call uiHelpGoto
    ret c
    call uiHelpNextRow
    ld a,(uiHelpError)
    or a
    ret z
    scf
    ret
uiHelpNextRow:
    call uiHelpLine
    ret c
    ld hl,(uiHelpStreamLine)
    inc hl
    ld (uiHelpStreamLine),hl
    or a
    ret

; Save a 32-bit file position every eight rows during the initial count.
; The last checkpoint also serves arbitrarily long files.
uiHelpCheckpoint:
    ld a,h
    or a
    ret nz
    ld a,l
    cp UI_HELP_INDEX_LAST+1
    ret nc
    and 7
    ret nz
    ld a,l
    call uiHelpIndexAddress
    push hl
    ld hl,(flHandle+FH_POSITION)
    ld a,(uiHelpRemaining)
    ld e,a
    ld d,0
    call bios_sub_hl_de
    ex de,hl
    pop hl
    ld (hl),e
    inc hl
    ld (hl),d
    inc hl
    ex de,hl
    ld hl,(flHandle+FH_POSITION+2)
    jp nc,uiHelpCheckpointHigh
    dec hl
uiHelpCheckpointHigh:
    ex de,hl
    ld (hl),e
    inc hl
    ld (hl),d
    ret
uiHelpIndexAddress:
    rrca
    ld e,a
    ld d,0
    ld hl,uiHelpIndex
    add hl,de
    ret

; Advance the current buffered stream when close enough. Otherwise seek to
; the latest checkpoint at or before the requested row, then skip its tail.
uiHelpGoto:
    ld (uiHelpTarget),hl
    ex de,hl
    ld hl,(uiHelpStreamLine)
    ex de,hl
    call bios_sub_hl_de
    jp c,uiHelpGotoIndexed
    ld a,h
    or a
    jp nz,uiHelpGotoIndexed
    ld a,l
    cp 8
    jp c,uiHelpSkip
uiHelpGotoIndexed:
    ld hl,(uiHelpTarget)
    ld de,UI_HELP_INDEX_LAST
    call bios_cmp_hl_de
    jp c,uiHelpGotoCheckpoint
    ex de,hl
uiHelpGotoCheckpoint:
    ld a,l
    and 0F8h
    ld l,a
    ld h,0
    ld (uiHelpStreamLine),hl
    call uiHelpIndexAddress
    ld d,h
    ld e,l
    inc de
    inc de
    inc de
    inc de
    ld bc,flHandle+FH_SEEK
    call bios_memcpy_bc_hl
    xor a
    ld (uiHelpRemaining),a
    ld a,FILE_OP_SEEK
    call flCall
    ret c
    ld hl,(uiHelpStreamLine)
    ex de,hl
    ld hl,(uiHelpTarget)
    call bios_sub_hl_de
uiHelpSkip:
    ld a,h
    or l
    ret z
    push hl
    call uiHelpNextRow
    pop hl
    jp c,uiHelpSkipEnd
    dec hl
    jp uiHelpSkip
uiHelpSkipEnd:
    ld a,(uiHelpError)
    or a
    ret z
    scf
    ret

; Return one padded row, CF only if no more rows. Read chunks on demand.
uiHelpLine:
    ld hl,uiHelpRow
    ld b,UI_HELP_COLUMNS
uiHelpPad:
    ld (hl),' '
    inc hl
    dec b
    jp nz,uiHelpPad
    ld hl,uiHelpRow
    ld c,0
uiHelpCharacter:
    call uiHelpByte
    jp c,uiHelpLineEnd
    cp 10
    ret z
    cp 13
    jp z,uiHelpCharacter
    ld b,a
    ld a,c
    cp UI_HELP_COLUMNS
    jp nc,uiHelpCharacter
    ld a,b
    cp 9
    jp z,uiHelpTab
    cp 32
    jp nc,uiHelpStore
    ld a,' '
uiHelpStore:
    ld (hl),a
    inc hl
    inc c
    jp uiHelpCharacter
uiHelpTab:
    inc hl
    inc c
    ld a,c
    cp 13
    jp c,uiHelpTab
    jp z,uiHelpCharacter
    cp 26
    jp z,uiHelpCharacter
    cp UI_HELP_COLUMNS
    jp c,uiHelpTab
    jp uiHelpCharacter
uiHelpLineEnd:
    ld a,c
    or a
    ret nz
    scf
    ret

; Carry on EOF or on a recorded I/O error. Preserve BC/DE/HL.
uiHelpByte:
    push hl
    ld a,(uiHelpRemaining)
    or a
    jp nz,uiHelpTake
    ld a,(uiHelpError)
    or a
    jp nz,uiHelpEof
    ld a,(flHandle+FH_EOF)
    or a
    jp nz,uiHelpEof
    push bc
    push de
    ld hl,UI_HELP_BUFFER
    ld (flHandle+FH_BUFFER),hl
    ld hl,UI_HELP_BLOCK
    ld (flHandle+FH_COUNT),hl
    ld a,FILE_OP_READ
    call flCall
    pop de
    pop bc
    jp nc,uiHelpFilled
    ld (uiHelpError),a
    jp uiHelpEof
uiHelpFilled:
    ld hl,UI_HELP_BUFFER
    ld (uiHelpNext),hl
    ld a,(flHandle+FH_COUNT)
    or a
    jp z,uiHelpEof
uiHelpTake:
    dec a
    ld (uiHelpRemaining),a
    ld hl,(uiHelpNext)
    ld a,(hl)
    inc hl
    ld (uiHelpNext),hl
    pop hl
    or a
    ret
uiHelpEof:
    pop hl
    scf
    ret

uiHelpTitle: DB 'Help',0
uiHelpOk: DB 'OK [Enter]',0
uiHelpDescriptor:
    UI_MODAL_BEGIN UI_HELP_BODY,1,1
    UI_MODAL_BUTTON uiHelpOk,0,0,1
    UI_MODAL_END
uiHelpScroll: UI_SCROLLBAR 304,UI_HELP_Y,UI_HELP_BODY,8,3,0,UI_HELP_ROWS,0
uiHelpLast: DW 0
uiHelpTarget: DW 0
uiHelpStreamLine: DW 0
uiHelpIndex: DS UI_HELP_INDEX_BYTES,0
uiHelpNext: DW 0
uiHelpRemaining: DB 0
uiHelpError: DB 0
uiHelpRow: DS UI_HELP_COLUMNS,0
