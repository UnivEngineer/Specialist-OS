; Column-at-a-time scalar from the existing five 2 KiB phase planes.
; Ordinary KOI8 only, matching the standalone grid. No screen staging.
    MODULE columnPre
build:
    ld de,03000h
    xor a
buildPhase:
    ld (operation+1),a
    ld hl,FONT_ADDR
    ld bc,2048
buildByte:
    ld a,(hl)
    push bc
    ld c,a
operation: ld a,0
    or a
    ld a,c
    jp z,left2
    ld a,(operation+1)
    cp 4
    ld a,c
    jp z,left4
    rrca
    rrca
    ld b,a
    ld a,(operation+1)
    cp 1
    ld a,b
    jp z,right2
    ld a,(operation+1)
    cp 2
    ld a,b
    jp z,left6
    rrca
    rrca
    and 3
    jp store
left2:
    add a,a
    add a,a
    jp store
left4:
    add a,a
    add a,a
    add a,a
    add a,a
    jp store
right2:
    and 0Fh
    jp store
left6:
    and 0C0h
store:
    ld (de),a
    inc de
    inc hl
    pop bc
    dec bc
    ld a,b
    or c
    jp nz,buildByte
    ld a,(operation+1)
    inc a
    cp 5
    jp nz,buildPhase
    ret
scalar:
    ld a,c
    ld (code+1),a
    ld hl,(bios_vars.cursorY)
    ld a,h
    and 3
    ld (phase+1),a
    ld a,h
    rrca
    rrca
    and 3Fh
    add a,90h
    ld d,a
    ld a,l
    sub 7
    ld e,a
    inc h
    inc h
    inc h
    ld (bios_vars.cursorY),hl
code: ld l,0
phase: ld a,0
    or a
    ld h,030h
    ld b,3
    jp z,column
    dec a
    ld h,FONT_ADDR >> 8
    ld b,0C0h
    jp z,column
    dec a
    jp nz,phase6
    ld h,038h
    ld b,0F0h
    push hl
    push de
    call column
    pop de
    pop hl
    inc d
    ld h,040h
    ld b,03Fh
    jp column
phase6:
    ld h,048h
    ld b,0FCh
    push hl
    push de
    call column
    pop de
    pop hl
    inc d
    ld h,050h
    ld b,0Fh
column:
    DUP 7
        ld a,(de)
        and b
        or (hl)
        ld (de),a
        inc h
        inc de
    EDUP
    ld a,(de)
    and b
    or (hl)
    ld (de),a
    ret
    ENDMODULE
