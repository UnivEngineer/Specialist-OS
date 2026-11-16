; CP/M repeated-nibble packet, lifted from FontBen 4DUP.
; SP reads two character codes with POP HL. No CALL/PUSH while borrowed.
; BIOS/MXCALL executes with interrupts disabled. The caller stack is restored
; before controls, scrolling or return. Cursor and 16-bit remaining length
; are updated once per packet, never per pair. No pre-scan for a terminator.
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
    rrca
    and 127
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
    ld hl,0
    add hl,sp
    ld (savedSP+1),hl
    ld hl,(text)
    ld sp,hl
route: jp normalLim

    MACRO PAIR_LOOP label, kind, inverse
label:
    pop hl ; low=first code, high=second; both now in registers
    IF kind != 2
    ld a,l
    cp kind
    jp c,stopBeforePair
    ld a,h
    cp kind
    jp c,stopBeforePair
    ENDIF
    ld c,h
    ld h,FONT_ADDR/256
    ld b,h
    DUP 7
        ld a,(bc)
        xor (hl)
        and 0Fh
        xor (hl)
        IF inverse
        cpl
        ENDIF
        ld (de),a
        inc h
        inc b
        inc e
    EDUP
    ld a,(bc)
    xor (hl)
    and 0Fh
    xor (hl)
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
    PAIR_LOOP normalZ,32,0
    PAIR_LOOP normalRawZ,1,0
    PAIR_LOOP normalLim,2,0
    PAIR_LOOP inverseZ,32,1
    PAIR_LOOP inverseRawZ,1,1
    PAIR_LOOP inverseLim,2,1
stopBeforePair:
    dec sp
    dec sp ; unconsumed control/NUL pair goes to scalar frontend
finish:
    ld hl,0
    add hl,sp
savedSP: ld sp,0
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
