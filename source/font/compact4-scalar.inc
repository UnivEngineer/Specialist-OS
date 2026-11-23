; A = код. Запись двух соседних глифов занимает восемь байтов.
    MACRO COMPACT4_POINTER high, low, base
    rlca
    rlca
    ld high,a
    and 0F8h
    ld low,a
    ld a,high
    and 3
    add a,base/256
    ld high,a
    ENDM

; H = X, C = код, DE = верхний байт экрана. Разрушает AF/BC/DE/HL.
    ld a,h
    and 2
    ld b,a
    rrca
    xor c
    and 1
    push af
    ld a,c
    COMPACT4_POINTER h,l,FONT_ADDR
    ex de,hl
    ld a,(bios_vars.inverse)
    sub 1
    sbc a,a
    cpl
    ld c,a
    ld a,b
    or a
    ld b,0F0h
    jp z,compact4MaskReady
    ld b,0Fh
compact4MaskReady:
    pop af
    jp nz,compact4Rotated

    MACRO COMPACT4_SINGLE label, rotate
label:
    ld a,(de)
    IF rotate
    DUP 4
        rrca
    EDUP
    ENDIF
    xor c
    xor (hl)
    and b
    xor (hl)
    ld (hl),a
    inc l
    inc e
    ld a,e
    and 7
    jp nz,label
    ret
    ENDM
    COMPACT4_SINGLE compact4Direct,0
    COMPACT4_SINGLE compact4Rotated,1
