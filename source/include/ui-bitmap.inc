; Byte-aligned arbitrary bitmap capture/replay. No hardware color reads.
; Raw: HL=VRAM, DE=packed column-major pixels, B=width, C=height (0=no-op).
; BC preserved; HL advances B screen columns, DE advances B*C bytes.
; Save reads VRAM, restore writes with the caller's current palette.
uiBitmapSave:
    push hl
    ld hl,uiBitmap.saveColumn
    jp uiBitmap.raw
uiBitmapRestore:
    push hl
    ld hl,uiBitmap.restoreColumn
    MODULE uiBitmap
raw:
    ld (columnCall+1),hl
    pop hl
    push bc
    ld a,b
    or a
    jp z,rawDone
    ld a,c
    or a
    jp z,rawDone
rawColumn:
    push bc
    push hl
columnCall:
    call saveColumn
    pop hl
    pop bc
    inc h
    dec b
    jp nz,rawColumn
rawDone:
    pop bc
    ret
saveColumn:
    ld a,c
    and 7
    ld (saveTail+1),a
    ld a,c
    rrca
    rrca
    rrca
    and 31
    ld c,a
    jp z,saveTail
saveEight:
    DUP 8
        ld a,(hl)
        ld (de),a
        inc l
        inc de
    EDUP
    dec c
    jp nz,saveEight
saveTail:
    ld c,0
    ld a,c
    or a
    ret z
saveByte:
    ld a,(hl)
    ld (de),a
    inc l
    inc de
    dec c
    jp nz,saveByte
    ret
restoreColumn:
    ld a,c
    and 7
    ld (restoreTail+1),a
    ld a,c
    rrca
    rrca
    rrca
    and 31
    ld c,a
    jp z,restoreTail
restoreEight:
    DUP 8
        ld a,(de)
        ld (hl),a
        inc de
        inc l
    EDUP
    dec c
    jp nz,restoreEight
restoreTail:
    ld c,0
    ld a,c
    or a
    ret z
restoreByte:
    ld a,(de)
    ld (hl),a
    inc de
    inc l
    dec c
    jp nz,restoreByte
    ret
    ENDMODULE

; Colored request (12 bytes): screen DW, width DB, height DB, pixels DW,
; attributes DW, attributeCapacity DW, colorRunCallback DW.
; Save callback: HL=VRAM, C=remaining height -> A=color, C=run length 1..C;
; B/DE/HL preserved. Palette stream: {height,color} runs in column order.
; Save CF=1 if the attribute capacity is exhausted. No VRAM writes on save.
; Restore consumes a successful, unmodified snapshot; returns A=last palette.
; Zero dimensions: no-op. AF/BC/DE/HL destroyed. Ordinary SP, non-reentrant.
uiBitmapSaveColors:
    call uiBitmapColor.request
    ld a,b
    or a
    ret z
    ld a,c
    or a
    ret z
    call uiBitmapSave
    ld hl,(uiBitmapColor.callback)
    ld (uiBitmapColor.query+1),hl
    ld hl,(uiBitmapColor.attributes)
    ex de,hl
    ld hl,(uiBitmapColor.screen)
    MODULE uiBitmapColor
saveColumn:
    ld a,(height)
    ld (remaining),a
saveRun:
    ld a,(remaining)
    ld c,a
query:
    call 0
    ld (color),a
    ; Reserve one run pair before writing either byte.
    push hl
    ld hl,(capacity)
    ld a,h
    or a
    jp nz,room
    ld a,l
    cp 2
    jp c,full
room:
    dec hl
    dec hl
    ld (capacity),hl
    pop hl
    ld a,c
    ld (de),a
    inc de
    ld a,(color)
    ld (de),a
    inc de
    ld a,(remaining)
    sub c
    ld (remaining),a
    jp z,saveNextColumn
    ld a,l
    add a,c
    ld l,a
    jp saveRun
saveNextColumn:
    ld a,(screen)
    ld l,a
    inc h
    dec b
    jp nz,saveColumn
    xor a
    ret
full:
    pop hl
    scf
    ret

request:
    ld de,screen
    ld b,12
read:
    ld a,(hl)
    ld (de),a
    inc hl
    inc de
    dec b
    jp nz,read
    ld hl,(pixels)
    ex de,hl
    ld hl,(screen)
    ld a,(width)
    ld b,a
    ld a,(height)
    ld c,a
    ret
; Descriptor fields stay contiguous in their published order.
screen: DW 0
width: DB 0
height: DB 0
pixels: DW 0
attributes: DW 0
capacity: DW 0
callback: DW 0
remaining: DB 0
color: DB 0
    ENDMODULE

uiBitmapRestoreColors:
    call uiBitmapColor.request
    ld a,b
    or a
    ret z
    ld a,c
    or a
    ret z
    MODULE uiBitmapRestoreColor
column:
    ld a,(uiBitmapColor.height)
    ld (remaining),a
run:
    push hl
    ld hl,(uiBitmapColor.attributes)
    ld c,(hl)
    inc hl
    ld a,(hl)
    ld (color),a
    ld (IO_COLOR),a
    inc hl
    ld (uiBitmapColor.attributes),hl
    pop hl
    push bc
    push hl
    call uiBitmap.restoreColumn
    pop hl
    pop bc
    ld a,(remaining)
    sub c
    ld (remaining),a
    jp z,nextColumn
    ld a,l
    add a,c
    ld l,a
    jp run
nextColumn:
    ld a,(uiBitmapColor.screen)
    ld l,a
    inc h
    dec b
    jp nz,column
    ld a,(color)
    ret
remaining: DB 0
color: DB 0
    ENDMODULE
