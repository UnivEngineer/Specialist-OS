; MAIN-only geometry contract: FNT1 +5 is pixel width, +6 glyph height.
; BIOS line pitch remains 10; cursorX is in half-pixels. No DLL bitmap access.
; Re-entered after external programs, even for another font of the same width.
ncLayoutInit:
    ld hl,vars.rowColors
    ld b,6*P_FILE_LIST_H
    ld a,COLOR_FILE
ncRowColorsInit:
    ld (hl),a
    inc hl
    dec b
    jp nz,ncRowColorsInit
    xor a
    ld (ncToolbarValid),a
    ld (ncToolbarValid+1),a
    ld (ncCommandEditResume),a
    ld (ncCommandCursorKept),a
    ld a,(FONT_ENGINE_HEADER+6)
    ld (ncGlyphHeight),a
    ld a,(FONT_ENGINE_HEADER+5)
    ld hl,ncLayout6
    cp 4
    jp z,ncLayoutNarrow
    cp 8
    jp nz,ncLayoutCopy
    ld hl,ncLayout8
    jp ncLayoutCopy
ncLayoutNarrow:
    ld hl,ncLayout4
ncLayoutCopy:
    ld de,ncFontWidth
    ld b,ncLayoutEnd-ncFontWidth
ncLayoutByte:
    ld a,(hl)
    ld (de),a
    inc hl
    inc de
    dec b
    jp nz,ncLayoutByte
    jp ncDriveGlyphPadding
; width, columns, X stride/2, cell bytes, visible rows, header/status chars,
; status size X/2, prompt usable chars, input chars, error chars, divider list.
ncLayout4: DB 4,3,32,6,P_FILE_LIST_H*3,44,36,76,94,36,40
    DW ncDividers4
ncLayout8: DB 8,1,88,22,P_FILE_LIST_H,22,14,60,46,18,20
    DW ncDividers8
ncLayout6: DB 6,2,48,10,FILE_LIST_SIZE,28,20,68,62,24,26
    DW g_fileDividers
ncFontWidth: DB 6
ncColumns: DB 2
ncColumnStep: DB 48
ncCellBytes: DB 10
ncVisibleCount: DB FILE_LIST_SIZE
ncHeaderChars: DB 28
ncStatusChars: DB 20
ncStatusSizeX: DB 68
ncCommandChars: DB 62
ncInputChars: DB 24
ncErrorChars: DB 26
ncDividers: DW g_fileDividers
ncLayoutEnd:
ncGlyphHeight: DB 8
ncDrivePadLeft: DB 1
ncDrivePadRight: DB 0

; 4/6px glyphs have one empty left column. For RAW8 use the actual A..H
; glyphs, so shifting the 8px font's blank column needs no NC code change.
ncDriveGlyphPadding:
    ld a,1
    ld (ncDrivePadLeft),a
    xor a
    ld (ncDrivePadRight),a
    ld a,(ncFontWidth)
    cp 8
    ret nz
    ld hl,FONT_ADDR+'A'
    ld b,8
    ld c,0
ncDrivePaddingRow:
    push hl
    ld d,8
ncDrivePaddingGlyph:
    ld a,(hl)
    or c
    ld c,a
    inc l
    dec d
    jp nz,ncDrivePaddingGlyph
    pop hl
    inc h
    dec b
    jp nz,ncDrivePaddingRow
    ld a,c
    or a
    ret z
    xor a
    ld (ncDrivePadLeft),a
    ld a,c
    or a
    jp m,ncDrivePaddingRight
    ld a,1
    ld (ncDrivePadLeft),a
ncDrivePaddingRight:
    ld a,c
    and 1
    ret nz
    ld a,1
    ld (ncDrivePadRight),a
    ret

; A=characters -> half-pixels, BC/DE/HL preserved.
ncTextAdvance:
    push bc
    ld b,a
    ld a,(ncFontWidth)
    rrca
    ld c,a
    xor a
ncTextAdvanceLoop:
    add a,b
    dec c
    jp nz,ncTextAdvanceLoop
    pop bc
    ret
; A=character count, L=baseline -> centered on screen. BC/DE preserved.
ncCenterLine:
    call ncTextAdvance
    or a
    rra
    cpl
    add a,97
    add a,2
    and 0FCh
    ld h,a
    ld (bios_vars.cursorY),hl
    ret
ncSubVisible:
    push hl
    ld hl,ncVisibleCount
    sub (hl)
    pop hl
    ret
ncDrawFileDividers:
    ld hl,(ncDividers)
    jp draw
ncDividers4:
    G_LINE 0,4,208,184
    G_VLINE 0,64,7,201
    G_VLINE 0,128,7,201
    DB 0
ncDividers8:
    G_LINE 0,4,208,184
    DB 0

; A=visible slot -> DE=relative BIOS coordinates, BC/HL preserved.
ncSlotPosition:
    ld de,(P_FILE_LIST_X1<<8)+P_FILE_LIST_Y
ncSlotColumn:
    cp P_FILE_LIST_H
    jp c,ncSlotRow
    sub P_FILE_LIST_H
    push af
    ld a,(ncColumnStep)
    add a,d
    ld d,a
    pop af
    jp ncSlotColumn
ncSlotRow:
    push bc
    add a,a
    ld c,a
    add a,a
    add a,a
    add a,c
    add a,e
    ld e,a
    pop bc
    ret

; Non-6px layouts use complete indexed rows; 6px retains bitmap scrolling.
; Empty slots are erased too, so a shrinking directory cannot leave old names.
ncPrintAdaptiveList:
    ld a,(ncVisibleCount)
    ld b,a
    ld c,0
    ld hl,(vars.fileListPtr)
ncAdaptiveSlot:
    ld a,c
    call ncSlotPosition
    push bc
    push hl
    push de
    ex de,hl
    call setCursorPosPanel
    call ncClearCell
    pop de
    pop hl
    ld a,(hl)
    cp 0FFh
    jp z,ncAdaptiveEmpty
    call printPanelFile
    inc hl
ncAdaptiveEmpty:
    pop bc
    inc c
    dec b
    jp nz,ncAdaptiveSlot
    ret
; Erase one aligned cell at current BIOS position, including line spacing.
ncClearCell:
    ld hl,(bios_vars.cursorY)
    ld a,h
    rrca
    rrca
    add a,090h
    ld h,a
    ld a,l
    sub 8
    ld l,a
    ld a,(ncCellBytes)
    ld b,a
ncClearCellColumn:
    push hl
    ld c,10
    xor a
ncClearCellPixel:
    ld (hl),a
    inc l
    dec c
    jp nz,ncClearCellPixel
    pop hl
    inc h
    dec b
    jp nz,ncClearCellColumn
    ret
