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
    ld a,(ncShiftState)
    call ncToolbarGetText
    ld b,COLOR_HELP_F
    ld c,COLOR_HELP_TEXT
    call uiToolbar
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
