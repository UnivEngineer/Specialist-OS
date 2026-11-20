; File panel status built as a single fixed-width raw text record.
printInfoLine:
    IF NC_STATUS_DEFER
        xor a
        ld (ncStatusPending),a
    ENDIF
        ld hl,state.panelA_filesCnt
        call readBytePanel
        or a
        jp z,printInfoEmpty

        call getSelectedFile
        push hl
        ld de,FILE_INFO.attrib
        add hl,de
        ld a,(hl)
        and 10h
        pop hl
        jp z,ncStatusFile
        ld a,(hl)
        cp 05Ch
        ld de,infoRootText
        jp z,ncStatusAction
        ld a,(hl)
        cp '.'
        jp nz,ncStatusDirectory
        inc hl
        ld a,(hl)
        dec hl
        cp '.'
        jp nz,ncStatusDirectory
        push hl
        inc hl
        inc hl
        ld a,(hl)
        pop hl
        cp ' '
        ld de,infoParentText
        jp z,ncStatusAction
ncStatusDirectory:
        ld de,ncTextBuffer
        call compactName
        call ncStatusPad
        ld hl,infoDirectoryText
        ld c,5
        jp ncStatusRight
ncStatusFile:
        ld de,ncTextBuffer
        call compactName
        push hl
        call ncStatusPad
        pop hl
        push de
        ld de,FILE_INFO.size
        add hl,de
        ld e,(hl)
        inc hl
        ld d,(hl)
        call ncFormatDecimal
        ld a,(ncDecimalCount)
        ld c,a
        pop de
ncStatusRight:
        ; DE=end of the padded line, C=length of the right-hand field.
        ld a,e
        sub c
        ld e,a
        jp nc,ncStatusRightReady
        dec d
ncStatusRightReady:
        ld b,c
        call ncStatusCopy
        jp ncStatusDraw

ncStatusAction:
        ex de,hl
        ld de,ncTextBuffer
        ld b,16 ; Both messages have 16 characters.
        call ncStatusCopy
        jp ncStatusFinish
printInfoEmpty:
        ld de,ncTextBuffer
ncStatusFinish:
        call ncStatusPad
        jp ncStatusDraw
ncStatusCopy:
        ld a,(hl)
        ld (de),a
        inc hl
        inc de
        dec b
        jp nz,ncStatusCopy
        ret
ncStatusPad:
        ld a,(ncStatusChars)
        ld l,a
        ld h,0
        ld bc,ncTextBuffer
        add hl,bc
        ld a,l
        sub e
        ld b,a
        ret z
        ld a,' '
ncStatusPadByte:
        ld (de),a
        inc de
        dec b
        jp nz,ncStatusPadByte
        ret
ncStatusDraw:
        call ncSetPanelBorder
        ld hl,(P_FILE_NAME_X << 8)+P_FILE_NAME_Y
        call setCursorPosPanel
        ld a,(ncStatusChars)
        ld c,a
        ld b,0
        ld hl,ncTextBuffer
        jp bios_printStringRawLim

infoDirectoryText: DB '<DIR>'
infoRootText: DB 'Go to drive root'
infoParentText: DB 'Go to parent dir'

    IF NC_STATUS_DEFER
ncStatusPending: DB 0

; Refresh after release while preserving the command-line caret and palette.
ncStatusIdle:
        ld a,(ncStatusPending)
        or a
        ret z
        ld a,(IO_KEYB_B)
        or 3
        inc a
        ret nz
        push bc
        push de
        push hl
        ld hl,(bios_vars.cursorY)
        push hl
        ld a,(ncCurrentColor)
        push af
        call printInfoLine
        pop af
    IF ENABLE_COLOR
        call ncSetColor
    ENDIF
        pop hl
        ld (bios_vars.cursorY),hl
    IF NC_BANKED
        call ncBankFlush
    ENDIF
        pop hl
        pop de
        pop bc
        ret
    ENDIF
