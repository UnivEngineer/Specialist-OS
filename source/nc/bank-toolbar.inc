; NC owns the toolbar templates, validity flags, bitmaps and palette runs.
drawToolbar:
    call ncBankReadShift
ncBankDrawToolbarState:
    ld (ncShiftState),a
    push bc
    push de
    push hl
    push af
    ld hl,(ncBankInverse)
    push hl
    ld hl,0
    ld (ncBankInverse),hl
    ld hl,ncToolbarValid
    ld de,ncToolbarNormalBitmap
    or a
    jp nz,ncBankToolbarSlot
    inc hl
    ld de,ncToolbarShiftBitmap
ncBankToolbarSlot:
    ld a,(hl)
    or a
    ex de,hl
    jp nz,ncBankToolbarReplay
    ld a,1
    ld (de),a
    push hl
    ld a,(ncShiftState)
    call ncToolbarGetText
    ld d,COLOR_HELP_F
    ld e,COLOR_HELP_TEXT
    ld a,GFX_TOOLBAR
    ld (ncQOperation),a
    call ncQueueCommand
    pop hl
    push hl
    ld de,4
    add hl,de
    ld e,(hl)
    inc hl
    ld d,(hl)
    ld hl,090F7h
    ld bc,48*256+9
    call ncSaveBitmap
    pop hl
    ld de,6
    add hl,de
    ld e,(hl)
    inc hl
    ld d,(hl)
    ld hl,ncToolbarNormalColors
    ld a,(ncShiftState)
    or a
    jp nz,ncBankToolbarStoreColors
    ld hl,ncToolbarShiftColors
ncBankToolbarStoreColors:
    ld bc,96
    call ncStoreBytes
    jp ncBankToolbarDone
ncBankToolbarReplay:
    call uiBitmapRestoreColors
ncBankToolbarDone:
    pop hl
    ld (ncBankInverse),hl
    pop af
    pop hl
    pop de
    pop bc
    ret
ncToolbarValid: DB 0,0
ncToolbarNormalBitmap:
    DW 090F7h
    DB 48,9
    DW NC_MAIN_TOOLBAR,NC_MAIN_TOOLBAR_COLORS,96,0
ncToolbarShiftBitmap:
    DW 090F7h
    DB 48,9
    DW NC_MAIN_TOOLBAR+48*9,NC_MAIN_TOOLBAR_COLORS+96,96,0
    MACRO NC_TOOLBAR_COLORS count
    DUP count
        DB 9,COLOR_HELP_F,9,COLOR_HELP_F
        DB 9,COLOR_HELP_TEXT,9,COLOR_HELP_TEXT,9,COLOR_HELP_TEXT,9,COLOR_HELP_TEXT
    EDUP
    ENDM
ncToolbarNormalColors:
    NC_TOOLBAR_COLORS 8
ncToolbarShiftColors:
    NC_TOOLBAR_COLORS 7
    DB 9,COLOR_HELP_F,9,COLOR_HELP_F,9,COLOR_HELP_F
    DB 9,COLOR_HELP_TEXT,9,COLOR_HELP_TEXT,9,COLOR_HELP_TEXT
