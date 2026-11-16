; Two lazy 432-byte snapshots, outside the file image. First paint goes through
; the installed font, then captures VRAM. Cache replay writes the same per-field
; colors: hardware does not provide a readable attribute framebuffer.
drawToolbar:
    ld a,(IO_KEYB_B)
    and 2
    ld (ncShiftState),a
    ld hl,ncToolbarValid
    ld de,NC_TOOLBAR_CACHE
    jp nz,ncToolbarSelect
    inc hl
    ld de,NC_TOOLBAR_CACHE+NC_TOOLBAR_CACHE_BYTES
ncToolbarSelect:
    push hl
    ex de,hl
    ld (ncToolbarBuffer),hl
    pop hl
    ld a,(hl)
    or a
    jp nz,ncToolbarReplay
    ld (hl),1
    ; Field lists depend on both geometry and Shift, never on DLL encoding.
    ld hl,ncToolbar6
    ld a,(ncFontWidth)
    cp 4
    jp nz,ncToolbarNot4
    ld hl,ncToolbar4
ncToolbarNot4:
    cp 8
    jp nz,ncToolbarFontReady
    ld hl,ncToolbar8
ncToolbarFontReady:
    ld a,(ncShiftState)
    or a
    jp nz,ncToolbarListReady
    inc hl
    inc hl
ncToolbarListReady:
    ld e,(hl)
    inc hl
    ld d,(hl)
    ex de,hl
    ld a,090h
    ld (ncToolbarColumn),a
ncToolbarField:
    ld a,(hl)
    inc hl
    or a
    jp z,ncToolbarCapture
    ld (ncToolbarFieldSpec),a
    and 7Fh
    ld (ncToolbarFieldWidth),a
    ld b,a
    push hl
    ; Fill complete field with the final background and attribute.
    call ncToolbarColor
    ld a,(ncToolbarColumn)
    ld h,a
ncToolbarFillColumn:
    ld l,247
    ld c,9
    ld a,(bios_vars.inverse)
ncToolbarFillRow:
    ld (hl),a
    inc l
    dec c
    jp nz,ncToolbarFillRow
    inc h
    dec b
    jp nz,ncToolbarFillColumn
    pop hl
    push hl
    ld b,0
ncToolbarLength:
    ld a,(hl)
    or a
    jp z,ncToolbarMeasured
    inc b
    inc hl
    jp ncToolbarLength
ncToolbarMeasured:
    ld a,b
    call ncTextAdvance
    ld c,a
    ld a,(ncToolbarFieldWidth)
    add a,a
    add a,a
    sub c
    rra
    ld c,a
    ld a,(ncToolbarColumn)
    sub 90h
    add a,a
    add a,a
    add a,c
    ; Snap origin to the engine's 4/8-pixel cell; 6px accepts any X.
    ld h,a
    ld a,(ncFontWidth)
    cp 6
    ld a,h
    jp z,ncToolbarXReady
    and 0FEh
    ld h,a
    ld a,(ncFontWidth)
    cp 4
    ld a,h
    jp z,ncToolbarXReady
    and 0FCh
ncToolbarXReady:
    ld h,a
    ld l,255
    ld (bios_vars.cursorY),hl
    pop hl
    call bios_printStringReal
    inc hl
    ld a,(ncToolbarFieldWidth)
    ld b,a
    ld a,(ncToolbarColumn)
    add a,b
    ld (ncToolbarColumn),a
    jp ncToolbarField
ncToolbarColor:
    ld a,(ncToolbarFieldSpec)
    and 80h
    ld a,COLOR_HELP_F
    ld de,0
    jp z,ncToolbarSetColor
    ld a,COLOR_HELP_TEXT
    dec de
ncToolbarSetColor:
    ld (IO_COLOR),a
    push hl
    ex de,hl
    ld (bios_vars.inverse),hl
    pop hl
    ret
ncToolbarCapture:
    ld hl,(ncToolbarBuffer)
    ex de,hl
    ld hl,090F7h
    ld b,48
    ld c,9
    call uiBitmapSave
    ld hl,0
    ld (bios_vars.inverse),hl
    ret
ncToolbarReplay:
    ld hl,(ncToolbarBuffer)
    ex de,hl
    ld h,090h
    ld b,48
    ld c,0
ncToolbarReplayColumn:
    ld a,c
    cp 2
    ld a,COLOR_HELP_F
    jp c,ncToolbarReplayColor
    ld a,COLOR_HELP_TEXT
    ; Shift's final F10 field is three bytes, its label also three.
    push af
    ld a,h
    cp 0BCh
    jp nz,ncToolbarReplayPop
    ld a,(ncShiftState)
    or a
    jp nz,ncToolbarReplayPop
    pop af
    ld a,COLOR_HELP_F
    jp ncToolbarReplayColor
ncToolbarReplayPop:
    pop af
ncToolbarReplayColor:
    ld (IO_COLOR),a
    push bc
    ld l,247
    ld c,9
ncToolbarReplayRow:
    ld a,(de)
    ld (hl),a
    inc de
    inc l
    dec c
    jp nz,ncToolbarReplayRow
    pop bc
    inc h
    inc c
    ld a,c
    cp 6
    jp c,ncToolbarReplayNext
    ld c,0
ncToolbarReplayNext:
    dec b
    jp nz,ncToolbarReplayColumn
    ret
ncToolbarValid: DB 0,0
ncToolbarBuffer: DW 0
ncToolbarColumn: DB 0
ncToolbarFieldSpec: DB 0
ncToolbarFieldWidth: DB 0
ncToolbar4: DW ncToolbarNormal4,ncToolbarShift4
ncToolbar6: DW ncToolbarNormal6,ncToolbarShift6
ncToolbar8: DW ncToolbarNormal8,ncToolbarShift8
    MACRO NC_TOOLBAR_PAIR KEY,TEXT
    UI_TOOLBAR_FIELD 2,0,KEY
    UI_TOOLBAR_FIELD 4,080h,TEXT
    ENDM
ncToolbarNormal4:
    NC_TOOLBAR_PAIR "F1","LeftDisk"
    NC_TOOLBAR_PAIR "F2","RightDsk"
    NC_TOOLBAR_PAIR "F3","DiskInfo"
    NC_TOOLBAR_PAIR "F4","EditFile"
    NC_TOOLBAR_PAIR "F5","CopyFile"
    NC_TOOLBAR_PAIR "F6","MoveFile"
    NC_TOOLBAR_PAIR "F7","MakeDir"
    NC_TOOLBAR_PAIR "F8","Delete"
    DB 0
ncToolbarShift4:
    NC_TOOLBAR_PAIR "F1","SortLeft"
    NC_TOOLBAR_PAIR "F2","SortRght"
    NC_TOOLBAR_PAIR "F3","DiskInfo"
    NC_TOOLBAR_PAIR "F4","NewFile"
    NC_TOOLBAR_PAIR "F5","CopyFile"
    NC_TOOLBAR_PAIR "F6","Rename"
    NC_TOOLBAR_PAIR "F7","MakeDir"
    UI_TOOLBAR_FIELD 3,0,"F10"
    UI_TOOLBAR_FIELD 3,080h,"Exit"
    DB 0
ncToolbarNormal6:
ncToolbarNormal8:
    NC_TOOLBAR_PAIR "F1","Left"
    NC_TOOLBAR_PAIR "F2","Rght"
    NC_TOOLBAR_PAIR "F3","Info"
    NC_TOOLBAR_PAIR "F4","Edit"
    NC_TOOLBAR_PAIR "F5","Copy"
    NC_TOOLBAR_PAIR "F6","Move"
    NC_TOOLBAR_PAIR "F7","MkDr"
    NC_TOOLBAR_PAIR "F8","Del"
    DB 0
ncToolbarShift6:
ncToolbarShift8:
    NC_TOOLBAR_PAIR "F1","Sort"
    NC_TOOLBAR_PAIR "F2","Sort"
    NC_TOOLBAR_PAIR "F3","Info"
    NC_TOOLBAR_PAIR "F4","New"
    NC_TOOLBAR_PAIR "F5","Copy"
    NC_TOOLBAR_PAIR "F6","Ren"
    NC_TOOLBAR_PAIR "F7","MkDr"
    UI_TOOLBAR_FIELD 3,0,"F10"
    UI_TOOLBAR_FIELD 3,080h,"Ex"
    DB 0
