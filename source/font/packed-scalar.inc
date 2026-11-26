scalar:
    ld a,(bios_vars.inverse)
    or a
    jp nz,packedInverse.scalar
    jp packedNormal.scalar
packedWrap:
    ld c,10
    call bios_fontControl
    ld hl,(bios_vars.cursorY)
    ret
    MODULE packedNormal
scalar:
    ld a,c
    ld (code+1),a
    ld hl,(bios_vars.cursorY)
    ld a,h
    cp 190
    call nc,packedWrap
    ld a,h
    and 3
    ld (phase+1),a
    ld a,h
    rrca
    rrca
    and 3Fh
    add a,high MX_SCREEN_BASE
    ld d,a
    ld a,l
    sub 7
    ld e,a
    inc h
    inc h
    inc h
    ld (bios_vars.cursorY),hl
    ex de,hl
code: ld e,0
phase: ld a,0
    or a
page0_0: ld d,0
    ld b,0FCh
    jp z,column
    dec a
page2_0: ld d,0
    ld b,03Fh
    jp z,column
    dec a
    jp nz,phase6
page1_0: ld d,0
    ld b,0Fh
    push hl
    push de
    call column
    pop de
    pop hl
    inc h
page2_1: ld d,0
    ld b,0C0h
    jp column
phase6:
page0_1: ld d,0
    ld b,3
    push hl
    push de
    call column
    pop de
    pop hl
    inc h
page1_1: ld d,0
    ld b,0F0h
column:
    DUP 7
        ld a,(de)
        xor (hl)
        and b
        xor (hl)
        ld (hl),a
        inc d
        inc hl
    EDUP
    ld a,(de)
    xor (hl)
    and b
    xor (hl)
    ld (hl),a
    ret
    ENDMODULE
    MODULE packedInverse
scalar:
    ld a,c
    ld (code+1),a
    ld hl,(bios_vars.cursorY)
    ld a,h
    cp 190
    call nc,packedWrap
    ld a,h
    and 3
    ld (phase+1),a
    ld a,h
    rrca
    rrca
    and 3Fh
    add a,high MX_SCREEN_BASE
    ld d,a
    ld a,l
    sub 7
    ld e,a
    inc h
    inc h
    inc h
    ld (bios_vars.cursorY),hl
    ex de,hl
code: ld e,0
phase: ld a,0
    or a
page0_0: ld d,0
    ld b,0FCh
    jp z,column
    dec a
page2_0: ld d,0
    ld b,03Fh
    jp z,column
    dec a
    jp nz,phase6
page1_0: ld d,0
    ld b,0Fh
    push hl
    push de
    call column
    pop de
    pop hl
    inc h
page2_1: ld d,0
    ld b,0C0h
    jp column
phase6:
page0_1: ld d,0
    ld b,3
    push hl
    push de
    call column
    pop de
    pop hl
    inc h
page1_1: ld d,0
    ld b,0F0h
column:
    DUP 7
        ld a,(de)
        xor (hl)
        and b
        xor (hl)
        xor b
        ld (hl),a
        inc d
        inc hl
    EDUP
    ld a,(de)
    xor (hl)
    and b
    xor (hl)
    xor b
    ld (hl),a
    ret
    ENDMODULE
