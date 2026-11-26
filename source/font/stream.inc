; Plan a whole aligned run before borrowing SP. Each pass writes one
; horizontal pixel row of the run. The scalar frontend handles partial cells.
; HL = text. BC = remaining length for Lim and RawLim. AF/BC/DE/HL are volatile.
; CF clear: HL points past the rendered run. CF set: HL is unchanged.
    MODULE rowStream
tryGroup:
    ld (text),hl
    ld a,(bios_vars.cursorX)
    cp 192
    jp nc,empty
    and 3
    jp nz,empty
    IFNDEF CELL_WIDTH
    ld a,(bios_vars.cursorY)
    cp 7
    jp c,empty
    ENDIF
    ld a,(fast6.realMode)
    add a,a
    jp nz,streamCapacity
    ld a,(bios_upper.escMode)
    inc a
    jp nz,empty
streamCapacity:
    ld a,(bios_vars.cursorX)
    rrca
    rrca
    add a,high MX_SCREEN_BASE
    ld (streamStart),a
    ld d,a
    ld a,high MX_SCREEN_END
    sub d
    IFDEF CELL_WIDTH
    add a,a
    ld e,a
    ELSE
    ld e,a
    ld d,0
    ld hl,streamCapacity6
    add hl,de
    ld e,(hl)
    ENDIF
    ld a,e
    or a
    jp z,empty
    ld a,(fast6.realMode)
    cp 2
    jp nc,streamLimited
    or a
    ld b,1
    jp nz,streamScanReady
    ld b,32
streamScanReady:
    ld hl,(text)
    ld c,0
streamScan:
    ld a,(hl)
    cp b
    jp c,streamPlanned
    inc hl
    inc c
    dec e
    jp nz,streamScan
    jp streamPlanned
streamLimited:
    ld a,b
    or a
    jp nz,streamUseCapacity
    ld a,c
    cp e
    jp c,streamCountReady
streamUseCapacity:
    ld c,e
streamCountReady:
    ld a,(fast6.realMode)
    add a,a
    jp nz,streamPlanned
    ld e,c
    ld b,32
    jp streamScanReady
streamPlanned:
    ld a,c
    IFDEF CELL_WIDTH
    and 0FEh
    ELSE
    and 0FCh
    ENDIF
    jp z,empty
    ld c,a
    ld b,0
    ld hl,(text)
    add hl,bc
    ld (streamNext),hl
    ld a,c
    rrca
    IFNDEF CELL_WIDTH
    rrca
    ld b,a
    add a,a
    add a,b
    ENDIF
    ld b,a
    ld a,(streamStart)
    add a,b
    ld (streamNormalLimit+1),a
    ld (streamInverseLimit+1),a
    xor a
    ld (streamRow),a
    ld a,(bios_vars.cursorY)
    sub 7
    ld e,a
    ld hl,0
    add hl,sp
    ld (streamSavedSP+1),hl
    ld a,(bios_vars.inverse)
    or a
    jp nz,streamInverse

    IFNDEF CELL_WIDTH
    INCLUDE "stream6-pixels.inc"
    ENDIF

    MACRO STREAM_RUN entry, inverse, limit
entry:
    ld a,(streamStart)
    ld d,a
    ld hl,(text)
    ld sp,hl
    ; No CALL or PUSH until streamFinish restores the caller's SP.
    IFDEF CELL_WIDTH
    IFDEF FONT4_COMPACT
    ld a,(streamRow)
    ld (.rowRight+1),a
    ld (.rowLeft+1),a
.group:
    pop hl
    ld a,h
    COMPACT4_POINTER b,c,FONT_ADDR
    ld a,c
.rowRight: or 0
    ld c,a
    ld a,h
    and 1
    ld a,(bc)
    jp nz,.rightReady
    DUP 4
    rrca
    EDUP
.rightReady:
    and 0Fh
    ld c,a
    ld a,l
    and 1
    ld b,a
    ld a,l
    COMPACT4_POINTER h,l,FONT_ADDR
    ld a,l
.rowLeft: or 0
    ld l,a
    ld a,b
    or a
    ld a,(hl)
    jp z,.leftReady
    DUP 4
    rlca
    EDUP
.leftReady:
    and 0F0h
    or c
    ELSE
    ld a,(streamRow)
    ld b,a
    add a,FONT_ADDR/256
    ld (.leftPage+1),a
    IFDEF FONT4_FAST
    ld a,(fontPreBase+1)
    add a,b
    ENDIF
    ld b,a
.group:
    pop hl
    ld c,h
.leftPage: ld h,0
    ld a,(bc)
    IFDEF FONT4_FAST
    or (hl)
    ELSE
    xor (hl)
    and 0Fh
    xor (hl)
    ENDIF
    ENDIF
    IF inverse
    cpl
    ENDIF
    ld (de),a
    inc d
    ELSE
    IF FONT_MODE
    ld a,(streamRow)
    ld b,a
    ld a,(fontPreBase+1)
    add a,b
    ld (.leftPage+1),a
    IF FONT_MODE == 2
    add a,24
    ld (.secondPage+1),a
    add a,8
    ld (.centerPage+1),a
    sub 24
    ld (.thirdPage+1),a
    add a,8
    ld (.rightPage+1),a
    ELSE
    add a,8
    ld (.centerPage+1),a
    ENDIF
    ld a,b
    add a,FONT_ADDR/256
    IF FONT_MODE == 2
    ld (.fourthPage+1),a
    ELSE
    ld (.rightPage+1),a
    ENDIF
.group:
    pop bc
    ld l,c
.leftPage: ld h,0
    ld a,(hl)
    ld l,b
    IF FONT_MODE == 2
.secondPage: ld h,0
    ENDIF
    STREAM6_MERGE 0FCh,inverse
    ld l,b
.centerPage: ld h,0
    ld a,(hl)
    pop bc
    ld l,c
    IF FONT_MODE == 2
.thirdPage: ld h,0
    ENDIF
    STREAM6_MERGE 0F0h,inverse
    ld l,c
.rightPage: ld h,0
    ld a,(hl)
    ld l,b
    IF FONT_MODE == 2
.fourthPage: ld h,0
    ENDIF
    STREAM6_MERGE 0C0h,inverse
    ELSE
    ld a,(streamRow)
    add a,FONT_ADDR/256
    ld h,a
.group:
    STREAM6_ROTATE inverse
    ENDIF
    ENDIF
    ld a,d
limit: cp 0
    jp nz,.group
    inc e
    ld hl,streamRow
    inc (hl)
    ld a,(hl)
    cp 8
    jp nz,entry
    jp streamFinish
    ENDM

    STREAM_RUN streamNormal,0,streamNormalLimit
    STREAM_RUN streamInverse,1,streamInverseLimit
streamFinish:
streamSavedSP: ld sp,0
    ld a,d
    sub high MX_SCREEN_BASE
    add a,a
    add a,a
    ld (bios_vars.cursorX),a
    cp 192
    jp c,streamReturn
    ld hl,(bios_vars.cursorY)
    call fontLineEnd
streamReturn:
    ld hl,(streamNext)
    or a
    ret
empty:
    ld hl,(text)
    scf
    ret
    IFNDEF CELL_WIDTH
streamCapacity6:
    DUP 49
    DB (($-streamCapacity6)/3)*4
    EDUP
    ENDIF
streamStart:    DB 0
streamRow:      DB 0
streamNext:     DW 0
text:           DW 0
    ENDMODULE
