; Value-only command emitters. The MAIN profile compiles the native routines.
    MACRO NC_GFX_STUB LABEL,OPERATION
LABEL:
    push af
    ld a,OPERATION
    ld (ncQOperation),a
    pop af
    jp ncQueueCommand
    ENDM
    NC_GFX_STUB uiDraw,GFX_DRAW
    NC_GFX_STUB uiFillRectPixels,GFX_FILL
    NC_GFX_STUB uiFrameRectPixels,GFX_FRAME
    NC_GFX_STUB uiInverseRectPixels,GFX_INVERSE
    NC_GFX_STUB uiFillRectBytes,GFX_BYTES
    NC_GFX_STUB uiInverseRect,GFX_INVERSE_BYTES
    NC_GFX_STUB uiCopyTextRect,GFX_COPY
    NC_GFX_STUB uiDrawCheckbox,GFX_CHECKBOX
    NC_GFX_STUB ncBankRecolor,GFX_RECOLOR
    NC_GFX_STUB uiRecolorRectBytes,GFX_RECOLOR_BYTES
    NC_GFX_STUB ncTileColumn,GFX_TILE_COLUMN
    NC_GFX_STUB uiBitmapRestoreColors,GFX_BITMAP_RESTORE_COLORS
uiInputFieldPixels:
    ld e,a
    ld a,d
    sub e
    ld d,a
    inc e
    inc e
    ld a,0FFh
    jp uiFillRectPixels
; uiClearWindow only calculates a rectangle; use the same source as MAIN.
    INCLUDE "../include/ui-clear-window.inc"

    NC_GFX_STUB ncSaveBitmap,GFX_BITMAP_SAVE
    NC_GFX_STUB ncStoreBytes,GFX_STORE

; Snapshot the source palette before moving the overlapping EXEC color table.
; The graphics executor receives each rectangle with its own palette bytes.
uiCopyTextRectColors:
    push bc
    push de
    push hl
    ld de,ncQColorRect
    ld b,6
ncQColorRectCopy:
    ld a,(hl)
    ld (de),a
    inc hl
    inc de
    dec b
    jp nz,ncQColorRectCopy
    or a
    jp z,ncQColorDone
    ld b,0
ncQColorRows:
    inc b
    sub 10
    jp nz,ncQColorRows
    ld a,b
    ld (ncQColorCount),a
    ld e,(hl)
    inc hl
    ld d,(hl)
    inc hl
    ld c,(hl)
    inc hl
    ld b,(hl)
    ex de,hl
    push hl
    push bc
    ld de,ncQColorRect+6
    ld a,(ncQColorCount)
    ld b,a
ncQPaletteCopy:
    ld a,(hl)
    ld (de),a
    inc hl
    inc de
    dec b
    jp nz,ncQPaletteCopy
    ld a,(ncQColorCount)
    add a,6
    ld c,a
    ld b,0
    ld hl,ncQColorRect
    ld a,GFX_COPY_COLORS
    ld (ncQOperation),a
    call ncQueueCommand
    pop bc
    pop hl
    push hl
    ld d,b
    ld e,c
    call ncBankCompare
    ld a,(hl)
    jp c,ncQColorLastReady
    jp z,ncQColorLastReady
    ld a,(ncQColorCount)
    dec a
    ld e,a
    ld d,0
    add hl,de
    ld a,(hl)
ncQColorLastReady:
    ld (ncQColorLast),a
    pop hl
    ld a,(ncQColorCount)
    ld e,a
    ld d,0
    push hl
    add hl,de
    ex de,hl
    pop hl
    call ncBankCopy.moveCore
ncQColorDone:
    pop hl
    pop de
    pop bc
    ld a,(ncQColorLast)
    ret
ncQColorCount: DB 0
ncQColorLast: DB COLOR_FILE
ncQColorRect: DS 26,0

; Palette runs are prepared in EXEC. The final pixels and runs are stored
; in NC's MAIN allocation before the window is drawn.
uiBitmapSaveColors:
    push bc
    push de
    push hl
    ld e,(hl)
    inc hl
    ld d,(hl)
    inc hl
    ld b,(hl)
    inc hl
    ld c,(hl)
    ld a,c
    ld (ncQSnapshotHeight),a
    ex de,hl
    ld de,NC_MAIN_PIXELS
    call ncSaveBitmap
    ld de,ncQSnapshot
ncQSnapshotColumn:
    push hl
    ld a,(ncQSnapshotHeight)
    ld (ncQSnapshotLeft),a
ncQSnapshotRun:
    ld a,(ncQSnapshotLeft)
    ld c,a
    call ncDialogColorRun
    push af
    ld a,d
    cp (ncQSnapshot+1024)>>8
    jp nc,ncQSnapshotFull
    ld a,c
    ld (de),a
    inc de
    pop af
    ld (de),a
    inc de
    ld a,l
    add a,c
    ld l,a
    ld a,(ncQSnapshotLeft)
    sub c
    ld (ncQSnapshotLeft),a
    jp nz,ncQSnapshotRun
    pop hl
    inc h
    dec b
    jp nz,ncQSnapshotColumn
    ld hl,ncQSnapshot
    push hl
    ex de,hl
    call ncBankSubtract
    ld b,h
    ld c,l
    pop hl
    ld de,NC_MAIN_COLORS
    call ncStoreBytes
    pop hl
    pop de
    pop bc
    or a
    ret
ncQSnapshotFull:
    pop af
    pop hl
    pop hl
    pop de
    pop bc
    scf
    ret
ncQSnapshotLeft: DB 0
ncQSnapshotHeight: DB 0
