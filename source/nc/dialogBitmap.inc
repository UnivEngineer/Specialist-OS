; One modal overlay at a time. Reuse radix scratch after closing any previous
; overlay; no catalog/index/toolbar storage is overwritten. Current windows
; lie in Y=80..175, above F3 driver metadata and below its header.

; HL=G_WINDOW list. Preserve all registers/flags, like ncDamageBoth callers.
ncDialogOpen:
    push af
    push bc
    push de
    push hl
    call ncDialogClose
    ld a,(ncCommandScreen)
    or a
    jp nz,ncDialogFallback ; Console output has no panel palette to snapshot.
    ld a,(ncCurrentColor)
    ld (ncDialogOldColor),a
    ld a,(hl)
    and 80h
    ld d,a
    inc hl
    ld a,(hl)
    ld (ncDialogRequest),a
    inc hl
    ld a,(hl)
    ld e,a
    ld a,d
    or a
    jp nz,ncDialogAbsolute
    ld a,(state.activePanel)
    or a
    ld a,e
    jp z,ncDialogScreenReady
    add a,24
    jp ncDialogScreenReady
ncDialogAbsolute:
    ld a,e
ncDialogScreenReady:
    ld (ncDialogRequest+1),a
    inc hl
    ld a,(hl)
    add a,6
    ld (ncDialogRequest+3),a
    inc hl
    ld a,(hl)
    add a,2
    ld (ncDialogRequest+2),a
    ; Fixed scratch budget: largest snapshot fits 2560 bytes, including the 33*76 overwrite prompt.
    ld hl,(ncDialogRequest+2)
    ld a,l
    cp 34
    jp nc,ncDialogFallback
    ld a,(ncDialogRequest)
    cp 80
    jp c,ncDialogFallback
    add a,h
    jp c,ncDialogFallback
    cp 177
    jp nc,ncDialogFallback
    ld a,h
    ld e,l
    ld d,0
    ld hl,0
ncDialogSize:
    add hl,de
    dec a
    jp nz,ncDialogSize
    ld de,2561
    call bios_cmp_hl_de
    jp nc,ncDialogFallback
    ld hl,ncDialogRequest
    call uiBitmapSaveColors
    jp c,ncDialogFallback
    ld a,1
    ld (ncDialogActive),a
    jp ncDialogOpenDone
ncDialogFallback:
    call ncDamageBoth
ncDialogOpenDone:
    pop hl
    pop de
    pop bc
    pop af
    ret

; Restore before any catalog operation can reuse radix scratch. AF/BC/DE/HL
; preserved, so this also serves as a tail return from confirmation dialogs.
ncDialogClose:
    push af
    ld a,(ncDialogActive)
    or a
    jp z,ncDialogCloseDone
    xor a
    ld (ncDialogActive),a
    push bc
    push de
    push hl
    ld hl,ncDialogRequest
    call uiBitmapRestoreColors
    ld a,(ncDialogOldColor)
    IF ENABLE_COLOR
    call ncSetColor
    ENDIF
    pop hl
    pop de
    pop bc
ncDialogCloseDone:
    pop af
    ret
ncDialogActive: DB 0
ncDialogOldColor: DB COLOR_FILE
ncDialogRequest:
    DW 0
    DB 0,0
    DW NC_DIALOG_PIXELS,NC_DIALOG_COLORS,NC_DIALOG_COLOR_BYTES,ncDialogColorRun

; Software palette lookup for file cells/frames and F3's memory/disk blocks.
; Empty blue pixels may use any foreground; all visible foregrounds and
; backgrounds are preserved. Return a bounded vertical run, never read color.
ncDialogColorRun:
    push bc
    push de
    push hl
    ld a,c
    ld (ncDialogRun),a
    ld a,h
    sub 090h
    ld c,0
    cp 24
    jp c,ncDialogPalettePanel
    sub 24
    inc c
ncDialogPalettePanel:
    ld (ncDialogByte),a
    ld e,a
    ld a,c
    ld (ncDialogPanel),a
    ld d,0
    push hl
    ld hl,state.panelA_info
    ld b,0
    add hl,bc
    ld a,(hl)
    pop hl
    or a
    jp nz,ncDialogInfoPalette
    ld a,e
    dec a
    cp 22 ; Frame bytes and unused gap bytes stay in the border palette.
    jp nc,ncDialogBorderPalette
    ld b,0
    ld d,a
    ld a,(ncColumnStep)
    rrca
    rrca
    ld e,a
    ld a,d
ncDialogPaletteColumn:
    sub e
    jp c,ncDialogPaletteColumnReady
    inc b
    jp ncDialogPaletteColumn
ncDialogPaletteColumnReady:
    add a,e
    ld (ncDialogCellByte),a
    ld d,a
    ld a,(ncCellBytes)
    cp d
    jp z,ncDialogBorderPalette
    jp c,ncDialogBorderPalette
    ld a,l
    sub SCROLL_TOP
    ld d,0
ncDialogPaletteRow:
    sub 10
    jp c,ncDialogPaletteRowReady
    inc d
    jp ncDialogPaletteRow
ncDialogPaletteRowReady:
    add a,10
    ld e,a
    ld a,10
    sub e
    ld (ncDialogRun),a
    ld a,b
    or a
    ld a,d
    jp z,ncDialogPaletteSlot
ncDialogPaletteSlotColumn:
    add a,P_FILE_LIST_H
    dec b
    jp nz,ncDialogPaletteSlotColumn
ncDialogPaletteSlot:
    ld (ncDialogSlot),a
    ld e,a
    ld a,(ncDialogPanel)
    or a
    ld a,e
    jp z,ncDialogPaletteIndex
    add a,3*P_FILE_LIST_H
ncDialogPaletteIndex:
    ld e,a
    ld d,0
    ld hl,vars.rowColors
    add hl,de
    ld a,(hl)
    IF NC_FILE_COLORS == 0
    ld a,COLOR_FILE
    ENDIF
    ld (ncDialogColor),a
    ld a,(ncCommandCursorKept)
    or a
    jp z,ncDialogPaletteFinish
    ld a,(state.activePanel)
    ld b,a
    ld a,(ncDialogPanel)
    cp b
    jp nz,ncDialogPaletteFinish
    ld e,a
    ld d,0
    ld hl,state.panelA_firstFile
    add hl,de
    ld b,(hl)
    inc hl
    inc hl
    ld a,(hl)
    sub b
    ld b,a
    ld a,(ncDialogSlot)
    cp b
    jp nz,ncDialogPaletteFinish
    ld a,(ncFontWidth)
    cp 6
    ld a,COLOR_CURSOR
    jp nz,ncDialogPaletteStore
    ld b,a
    ld a,(ncDialogCellByte)
    cp 9
    ld a,b
    jp nz,ncDialogPaletteStore
    and 0F0h
    or COLOR_FILE & 15
    jp ncDialogPaletteStore
ncDialogInfoPalette:
    ld a,e
    or a
    jp z,ncDialogBorderPalette
    cp 23
    jp z,ncDialogBorderPalette
    ld a,l
    cp 101
    jp c,ncDialogInfoNumber
    cp 126
    jp c,ncDialogInfoGap
    cp 145
    jp nc,ncDialogBorderPalette
    ld a,(pipHasVolume)
    or a
    jp z,ncDialogBorderPalette
    ld a,145
    jp ncDialogInfoNumberEnd
ncDialogInfoGap:
    ld a,126
    sub l
    ld (ncDialogRun),a
    jp ncDialogBorderPalette
ncDialogInfoNumber:
    ld a,101
ncDialogInfoNumberEnd:
    sub l
    ld (ncDialogRun),a
    ld a,e
    cp 8
    ld a,COLOR_INFONUMBER
    jp c,ncDialogPaletteStore
ncDialogBorderPalette:
    ld a,COLOR_BORDER
ncDialogPaletteStore:
    ld (ncDialogColor),a
ncDialogPaletteFinish:
    pop hl
    pop de
    pop bc
    ld a,(ncDialogRun)
    cp c
    jp nc,ncDialogPaletteReturn
    ld c,a
ncDialogPaletteReturn:
    ld a,(ncDialogColor)
    ret
ncDialogRun: DB 0
ncDialogColor: DB 0
ncDialogByte: DB 0
ncDialogCellByte: DB 0
ncDialogPanel: DB 0
ncDialogSlot: DB 0
