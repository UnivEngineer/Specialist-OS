; Aligned row-page 8x8 packet: FontBen register-resident text and video
; pointers. Quota <=48 is 8-bit; inversion, screen bounds, and 16-bit total
; are handled once per packet, outside the unrolled bitmap loop.
    MODULE fast6
tryGroup:
    ld (text),hl
    ld a,(bios_vars.cursorX)
    cp 192
    jp nc,empty
    and 3
    jp nz,empty
    ld a,(realMode)
    or a
    jp nz,escReady
    ld a,(bios_upper.escMode)
    inc a
    jp nz,empty
escReady:
    ld a,(bios_vars.cursorX)
    rrca
    rrca
    ld d,a
    ld a,48
    sub d
    ld e,a ; pairs remaining before the screen edge
    ld a,(realMode)
    cp 2
    jp nz,quotaReady
    ld a,b
    or a
    jp nz,quotaReady
    ld a,c
    cp e
    jp nc,quotaReady
    ld e,a
quotaReady:
    ld a,e
    or a
    jp z,empty
    ld (pairs),a
    ld a,d
    add a,90h
    ld d,a
    ld (startColumn+1),a
    ld a,(bios_vars.cursorY)
    sub 7
    ld e,a
    ld a,(realMode)
    add a,a
    ld l,a
    ld h,0
    ld bc,normalRoutes
    ld a,(bios_vars.inverse)
    or a
    jp z,routeReady
    ld bc,inverseRoutes
routeReady:
    add hl,bc
    ld a,(hl)
    ld (route+1),a
    inc hl
    ld a,(hl)
    ld (route+2),a
    ld hl,(text)
    ld b,h
    ld c,l
route: jp normalLim

    MACRO BYTE_LOOP label, kind, inverse
label:
    ld a,(bc)
    IF kind != 2
    cp kind
    jp c,finish
    ENDIF
    inc bc
    ld l,a
    ld h,FONT_ADDR/256
    DUP 7
        ld a,(hl)
        IF inverse
        cpl
        ENDIF
        ld (de),a
        inc h
        inc e
    EDUP
    ld a,(hl)
    IF inverse
    cpl
    ENDIF
    ld (de),a
    ld a,e
    sub 7
    ld e,a
    inc d
    ld hl,pairs
    dec (hl)
    jp nz,label
    jp finish
    ENDM
    BYTE_LOOP normalZ,32,0
    BYTE_LOOP normalRawZ,1,0
    BYTE_LOOP normalLim,2,0
    BYTE_LOOP inverseZ,32,1
    BYTE_LOOP inverseRawZ,1,1
    BYTE_LOOP inverseLim,2,1
finish:
    ld h,b
    ld l,c
    ld (text),hl
    ld a,d
startColumn: cp 0
    jp z,empty
    sub 90h
    add a,a
    add a,a
    ld (bios_vars.cursorX),a
    cp 192
    jp c,done
    ld hl,(bios_vars.cursorY)
    call bios_fontNewLine
done:
    ld hl,(text)
    or a
    ret
empty:
    ld hl,(text)
    scf
    ret
normalRoutes: DW normalZ,normalRawZ,normalLim
inverseRoutes: DW inverseZ,inverseRawZ,inverseLim
pairs: DB 0
text: DW 0
realMode: DB 0
    ENDMODULE
