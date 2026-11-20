; Colored text-cell copy. HL -> {source DW, destination DW, width DB,
; height DB, sourceColors DW, destinationColors DW}. Color tables have one
; byte per 10px row, in ascending Y order; height=0 or 10..200, width>0.
; Vertical overlap in either direction, horizontal only to the left.
; Same blank spacing/IRQ contracts as uiCopyTextRect. Color-table
; overlap must follow the same direction as VRAM overlap. No color reads.
; Adjacent equal colors share one stack-copy batch. AF/BC/DE/HL destroyed;
; A=last written color (undefined for height=0), SP restored. Non-reentrant.
uiCopyTextRectColors:
    MODULE uiTextColors
    ld de,source
    ld b,10
readRequest:
    ld a,(hl)
    ld (de),a
    inc hl
    inc de
    dec b
    jp nz,readRequest
    ld a,(height)
    or a
    ret z
    ld b,0
countRows:
    inc b
    sub 10
    jp nz,countRows
    ld a,b
    ld (rows),a
    ; Snapshot before updating an overlapping destination palette. Twenty
    ; bytes also let the inline kernel reread the same colors per column.
    ld hl,(sourceColors)
    ld de,colorsBuffer
snapshot:
    ld a,(hl)
    ld (de),a
    inc hl
    inc de
    dec b
    jp nz,snapshot
    ld hl,colorsBuffer
    ld (sourceColors),hl
    ld c,(hl)
    ld d,1
    ld a,(rows)
    ld b,a
    dec b
    jp z,selectDirection
countRuns:
    inc hl
    ld a,(hl)
    cp c
    jp z,sameRun
    ld c,a
    inc d
sameRun:
    dec b
    jp nz,countRuns
    ld a,d
    cp 3
    jp nc,inlineCopy ; Avoid a full copy setup for every short color run.
selectDirection:
    ld hl,(destination)
    ex de,hl
    ld hl,(source)
    ; Copy from the end when the destination follows the source.
    ld a,d
    cp h
    jp c,forward
    jp nz,backward
    ld a,e
    cp l
    jp c,forward
backward:
    ld a,1
    ld (direction),a
    ld a,(height)
    add a,l
    ld l,a
    ld (source),hl
    ld a,(height)
    add a,e
    ld e,a
    ex de,hl
    ld (destination),hl
    ld a,(rows)
    dec a
    ld e,a
    ld d,0
    ld hl,(sourceColors)
    add hl,de
    ld (sourceColors),hl
    ld hl,(destinationColors)
    add hl,de
    ld (destinationColors),hl
    jp nextRun
forward:
    xor a
    ld (direction),a
nextRun:
    ld hl,(sourceColors)
    ld a,(hl)
    ld (color),a
    ld b,0
extendRun:
    inc b
    ld hl,(destinationColors)
    ld a,(color)
    ld (hl),a
    ld a,(direction)
    or a
    jp nz,previousColor
    inc hl
    ld (destinationColors),hl
    ld hl,(sourceColors)
    inc hl
    jp colorAdvanced
previousColor:
    dec hl
    ld (destinationColors),hl
    ld hl,(sourceColors)
    dec hl
colorAdvanced:
    ld (sourceColors),hl
    ld a,(rows)
    dec a
    ld (rows),a
    jp z,copyRun
    ld a,(color)
    cp (hl)
    jp z,extendRun
copyRun:
    ld a,b
    add a,a
    ld c,a
    add a,a
    add a,a
    add a,c
    ld (runHeight),a
    ld c,a
    ld a,(direction)
    or a
    jp z,runPosition
    ld hl,(source)
    ld a,l
    sub c
    ld l,a
    ld (source),hl
    ld hl,(destination)
    ld a,l
    sub c
    ld l,a
    ld (destination),hl
runPosition:
    ld a,(color)
    ld (IO_COLOR),a
    ld hl,(destination)
    ld b,h
    ld c,l
    ld hl,(source)
    ld a,(width)
    ld d,a
    ld a,(runHeight)
    ld e,a
    call uiCopyTextRect
    ld a,(direction)
    or a
    jp nz,runDone
    ld a,(runHeight)
    ld hl,(source)
    add a,l
    ld l,a
    ld (source),hl
    ld a,(runHeight)
    ld hl,(destination)
    add a,l
    ld l,a
    ld (destination),hl
runDone:
    ld a,(rows)
    or a
    jp nz,nextRun
    ld a,(color)
    ret

; Fragmented colors: one unrolled stack pass, color chosen inside each row.
; No CALL while SP addresses VRAM. Palette copying uses the normal stack.
inlineCopy:
    ld hl,(destinationColors)
    ex de,hl
    ld hl,colorsBuffer
    ld a,(rows)
    ld b,a
copyPalette:
    ld a,(hl)
    ld (de),a
    inc hl
    inc de
    dec b
    jp nz,copyPalette
    ld hl,0
    add hl,sp
    ld (inlineRestoreSP+1),hl
    ld hl,(source)
    ld a,(destination+1)
    cp h
    jp c,inlineSetForward
    jp nz,inlineSetBackward
    ld a,(destination)
    cp l
    jp c,inlineSetForward
inlineSetBackward:
    ld a,1
    ld (direction),a
    jp inlineBackward
inlineSetForward:
    xor a
    ld (direction),a
inlineForward:
    ld hl,(source)
    inc l
    ld sp,hl
    ld hl,(destination)
    inc l
    ld bc,colorsBuffer
    ld a,(rows)
    ld (inlineForwardCount+1),a
inlineForwardRow:
    ld a,(bc)
    ld (IO_COLOR),a
    inc bc
    DUP 4
        pop de
        ld (hl),e
        inc l
        ld (hl),d
        inc l
    EDUP
    inc l
    inc l
    inc sp
    inc sp
inlineForwardCount:
    ld a,0
    dec a
    ld (inlineForwardCount+1),a
    jp nz,inlineForwardRow
    jp inlineNextColumn
inlineBackward:
    ld hl,(destination)
    ld a,(height)
    add a,l
    ld l,a
    dec l
    ld sp,hl
    ld hl,(source)
    ld a,(height)
    add a,l
    ld l,a
    dec l
    ld (inlineSource+1),hl
    ld a,(rows)
    dec a
    ld e,a
    ld d,0
    ld hl,colorsBuffer
    add hl,de
    ld b,h
    ld c,l
inlineSource:
    ld hl,0
    ld a,(rows)
    ld (inlineBackwardCount+1),a
inlineBackwardRow:
    ld a,(bc)
    ld (IO_COLOR),a
    dec bc
    DUP 4
        dec l
        ld d,(hl)
        dec l
        ld e,(hl)
        push de
    EDUP
    dec l
    dec l
    dec sp
    dec sp
inlineBackwardCount:
    ld a,0
    dec a
    ld (inlineBackwardCount+1),a
    jp nz,inlineBackwardRow
inlineNextColumn:
    ld hl,(source)
    inc h
    ld (source),hl
    ld hl,(destination)
    inc h
    ld (destination),hl
    ld a,(width)
    dec a
    ld (width),a
    jp z,inlineRestoreSP
    ld a,(direction)
    or a
    jp z,inlineForward
    jp inlineBackward
inlineRestoreSP:
    ld sp,0
    ld a,(direction)
    or a
    jp z,inlineLastColor
    inc bc
    ld a,(bc)
    ret
inlineLastColor:
    dec bc
    ld a,(bc)
    ret
; Request fields must stay contiguous and in this order.
source: DW 0
destination: DW 0
width: DB 0
height: DB 0
sourceColors: DW 0
destinationColors: DW 0
rows: DB 0
direction: DB 0
color: DB 0
runHeight: DB 0
colorsBuffer: DS 20,0
    ENDMODULE
