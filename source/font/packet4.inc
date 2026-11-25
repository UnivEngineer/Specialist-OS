; Пакет CP/M для двух знакомест в одном байте экрана.
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
    add a,a
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
    jp c,quotaReady
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
    ld a,d
    add a,90h
    ld d,a
    ld (startColumn+1),a
    ; D продвигается на каждом байте экрана. Для Lim задаём границу
    ; обычных петель, для RawLim — сырой. У RawZ граница всегда C0h.
    IFDEF FONT4_COMPACT
    ld a,d
    add a,e
    ld (compactLimit+1),a
    ld (compactUpLimit+1),a
    ELSE
    ld a,(realMode)
    cp 2
    jp z,rawBound
    add a,a
    jp nz,limitReady
    ld a,0C0h
    jp nc,zBoundReady
    ld a,d
    add a,e
zBoundReady:
    ld (normalZLimit+1),a
    ld (normalZReverseLimit+1),a
    ld (inverseZLimit+1),a
    ld (inverseZReverseLimit+1),a
    jp limitReady
rawBound:
    ld a,d
    add a,e
    ld (normalLimit+1),a
    ld (normalReverseLimit+1),a
    ld (inverseLimit+1),a
    ld (inverseReverseLimit+1),a
limitReady:
    ENDIF
    ld a,(bios_vars.cursorY)
    sub 7
    ld e,a
    IFDEF FONT4_COMPACT
    INCLUDE "compact4-packet.inc"
    ELSE
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

    MACRO PAIR_CODES kind, row
    pop hl ; L — левый глиф, H — правый.
    IF kind != 2
    ld a,l
    cp kind
    jp c,stopBeforePair
    ld a,h
    cp kind
    jp c,stopBeforePair
    ENDIF
    ld c,h
    IFDEF FONT4_FAST
    ld h,FONT_ADDR/256+row
    ELSE
    ld h,b
    ENDIF
    ENDM

    MACRO PAIR_ROWS inverse, reverse
    DUP 7
        PAIR_ROW inverse
        IF reverse
        dec h
        dec b
        dec e
        ELSE
        inc h
        inc b
        inc e
        ENDIF
    EDUP
    PAIR_ROW inverse
    ENDM

    MACRO PAIR_LOOP label, kind, inverse, limit, reverseLimit, page
label:
    IFDEF FONT4_FAST
page: ld b,0
    ELSE
    ld b,FONT_ADDR/256
    ENDIF
.forward:
    PAIR_CODES kind,0
    PAIR_ROWS inverse,0
    inc d
    ld a,d
    IF kind != 1
limit: cp 0
    ELSE
    cp 0C0h
    ENDIF
    jp z,finish
    ; E и B уже у последней строки, начинаем с неё следующую пару.
    PAIR_CODES kind,7
    PAIR_ROWS inverse,1
    inc d
    ld a,d
    IF kind != 1
reverseLimit: cp 0
    ELSE
    cp 0C0h
    ENDIF
    jp nz,.forward
    jp finish
    ENDM
    MACRO PAIR_ROW inverse
    ld a,(bc)
    IFDEF FONT4_FAST
    or (hl)
    ELSE
    xor (hl)
    and 0Fh
    xor (hl)
    ENDIF
    IF inverse
    cpl
    ENDIF
    ld (de),a
    ENDM
    PAIR_LOOP normalZ,32,0,normalZLimit,normalZReverseLimit,normalZRightPage
    PAIR_LOOP normalRawZ,1,0,unused,unused,normalRawZRightPage
    PAIR_LOOP normalLim,2,0,normalLimit,normalReverseLimit,normalLimRightPage
    PAIR_LOOP inverseZ,32,1,inverseZLimit,inverseZReverseLimit,inverseZRightPage
    PAIR_LOOP inverseRawZ,1,1,unused,unused,inverseRawZRightPage
    PAIR_LOOP inverseLim,2,1,inverseLimit,inverseReverseLimit,inverseLimRightPage
    ENDIF
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
    call fontLineEnd
done:
    ld hl,(text)
    or a
    ret
empty:
    ld hl,(text)
    scf
    ret
    IFNDEF FONT4_COMPACT
normalRoutes:   DW normalZ,normalRawZ,normalLim
inverseRoutes:  DW inverseZ,inverseRawZ,inverseLim
    ENDIF
text:           DW 0
realMode:       DB 0
    ENDMODULE
