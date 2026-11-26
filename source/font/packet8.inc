; Aligned row-page 8x8 packet: FontBen register-resident text and video
; pointers. Quota <=48 is 8-bit; inversion, screen bounds, and 16-bit total
; are handled once per packet, outside the unrolled bitmap loop.
    MODULE fast6
tryGroup:
    ld (text),hl
    ld a,(bios_vars.cursorX)
    cp 192
    jp nc,empty
    IFNDEF FONT_GRAPHIC_X
    and 3
    jp nz,empty
    ENDIF
    ld a,(realMode)
    add a,a
    jp nz,escReady
    ld a,(bios_upper.escMode)
    inc a
    jp nz,empty
escReady:
    IFDEF FONT_ALIGNED_ROWS
    jp graphicAlignedPacket
    ELSE
    ld a,(bios_vars.cursorX)
    IFDEF FONT_GRAPHIC_X
    and 3
    jp nz,graphicPacket
    ld a,(bios_vars.cursorX)
    ENDIF
    rrca
    rrca
    ld d,a
    ld a,48
    sub d
    ld e,a ; columns remaining before the screen edge
    ld a,(realMode)
    cp 2
    jp c,quotaReady
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
    ld a,d
    add a,high MX_SCREEN_BASE
    ld d,a
    ld (startColumn+1),a
    ld a,d
    add a,e
    ld (normalDownLimit+1),a
    ld (normalUpLimit+1),a
    ld (inverseDownLimit+1),a
    ld (inverseUpLimit+1),a
    ld a,(bios_vars.cursorY)
    sub 7
    ld e,a
    ld a,(realMode)
    cp 2
    jp z,unrestricted
    add a,a
    ld a,1
    jp nz,thresholdReady
    ld a,32
thresholdReady:
    ld (normalDownMin+1),a
    ld (normalUpMin+1),a
    ld (inverseDownMin+1),a
    ld (inverseUpMin+1),a
    ld hl,normalDown
    ld bc,normalUp
    ld a,(bios_vars.inverse)
    or a
    jp z,checkedRoute
    ld hl,inverseDown
    ld bc,inverseUp
checkedRoute:
    jp routeReady
unrestricted:
    ld hl,normalDownRaw
    ld bc,normalUpRaw
    ld a,(bios_vars.inverse)
    or a
    jp z,routeReady
    ld hl,inverseDownRaw
    ld bc,inverseUpRaw
routeReady:
    ld (route+1),hl
    ld (normalFromUp+1),hl
    ld (inverseFromUp+1),hl
    ld h,b
    ld l,c
    ld (normalFromDown+1),hl
    ld (inverseFromDown+1),hl
    ld hl,(text)
    ld b,h
    ld c,l
route: jp normalDownRaw

    MACRO BYTE_LOOP label, raw, threshold, inverse, up, limit, next
label:
    ld a,(bc)
threshold: cp 0
    jp c,finish
raw:
    ; RawLim enters here with no inspection of character codes.
    ld a,(bc)
    inc bc
    ld l,a
    IF up
    ld h,FONT_ADDR/256+7
    ELSE
    ld h,FONT_ADDR/256
    ENDIF
    DUP 7
        ld a,(hl)
        IF inverse
        cpl
        ENDIF
        ld (de),a
        IF up
        dec h
        dec e
        ELSE
        inc h
        inc e
        ENDIF
    EDUP
    ld a,(hl)
    IF inverse
    cpl
    ENDIF
    ld (de),a
    inc d
    ld a,d
limit: cp 0
next: jp nz,0
    jp finish
    ENDM
    BYTE_LOOP normalDown,normalDownRaw,normalDownMin,0,0,normalDownLimit,normalFromDown
    BYTE_LOOP normalUp,normalUpRaw,normalUpMin,0,1,normalUpLimit,normalFromUp
    BYTE_LOOP inverseDown,inverseDownRaw,inverseDownMin,1,0,inverseDownLimit,inverseFromDown
    BYTE_LOOP inverseUp,inverseUpRaw,inverseUpMin,1,1,inverseUpLimit,inverseFromUp
finish:
    ld h,b
    ld l,c
    ld (text),hl
    ld a,d
startColumn: cp 0
    jp z,empty
    sub high MX_SCREEN_BASE
    add a,a
    add a,a
    ld (bios_vars.cursorX),a
    cp 192
    jp c,done
    ld hl,(bios_vars.cursorY)
    call fontLineEnd
done:
    ld hl,(text)
    or a
    ret
    ENDIF
empty:
    ld hl,(text)
    scf
    ret
text:           DW 0
realMode:       DB 0
    ENDMODULE
