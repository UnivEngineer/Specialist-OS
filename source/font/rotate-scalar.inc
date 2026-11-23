; Shared rotating scalar for the paged and compact module. Layout is known
; at assembly time. Separate inverse kernels remove XOR 0 from normal rows.
; BC=screen, DE=glyph, H=next cursor X/2. All registers volatile.
    MODULE scalar6
scalar6Draw:
    ld a,h
    and 3
    ex de,hl
    ld e,a
    ld a,(bios_vars.inverse)
    or a
    jp nz,inverse
    ld a,e
    or a
    jp z,normal0
    dec a
    jp z,normal1
    dec a
    jp z,normal2
    jp normal3
inverse:
    ld a,e
    or a
    jp z,inverse0
    dec a
    jp z,inverse1
    dec a
    jp z,inverse2
    jp inverse3
    IFDEF FONT_KOI7
    IF K7_MODE == 1 || K7_MODE == 3
    DEFINE SCALAR6_PHASE_TABLES
    ENDIF
    ENDIF
    IFDEF SCALAR6_PHASE_TABLES
    IF K7_MODE == 1
    INCLUDE "koi7-packed-scalar.inc"
    ELSE
    INCLUDE "koi7-fast-scalar.inc"
    ENDIF
    ELSE
    MACRO STEP_GLYPH
    IFDEF SCALAR_LINEAR
    inc hl
    ELSE
    inc h
    ENDIF
    ENDM
    MACRO READ_SCALAR_ROW
    ld a,(hl)
    ENDM
    MACRO ONE shifts, keep, inv, advance
    ld a,(bc)
    and keep
    ld e,a
    READ_SCALAR_ROW
    IF inv
    xor 3Fh
    ENDIF
    IF shifts
    DUP shifts
        add a,a
    EDUP
    ENDIF
    xor e
    ld (bc),a
    IF advance
    STEP_GLYPH
    inc bc
    ENDIF
    ENDM
    MACRO TWO shifts, left, right, inv, advance
    READ_SCALAR_ROW
    IF inv
    xor 3Fh
    ENDIF
    DUP shifts
        rrca
    EDUP
    ld e,a
    ld a,(bc)
    xor e
    and left
    xor e
    ld (bc),a
    inc b
    ld a,(bc)
    xor e
    and right
    xor e
    ld (bc),a
    IF advance
    dec b
    STEP_GLYPH
    inc bc
    ENDIF
    ENDM
    MACRO ONE_CORE label, shifts, keep, inv
label:
    IFDEF FONT_KOI7
    IF K7_MODE != 3
    call .half
    ld h,K7_RAW/256
    ld a,l
    or 80h
    ld l,a
.half:
    DUP 4
        ONE shifts,keep,inv,1
    EDUP
    ret
    ELSE
    DUP 7
        ONE shifts,keep,inv,1
    EDUP
    ONE shifts,keep,inv,0
    ret
    ENDIF
    ELSE
    DUP 7
        ONE shifts,keep,inv,1
    EDUP
    ONE shifts,keep,inv,0
    ret
    ENDIF
    ENDM
    MACRO TWO_CORE label, shifts, left, right, inv
label:
    IFDEF FONT_KOI7
    IF K7_MODE != 3
    call .half
    ld h,K7_RAW/256
    ld a,l
    or 80h
    ld l,a
.half:
    DUP 4
        TWO shifts,left,right,inv,1
    EDUP
    ret
    ELSE
    DUP 7
        TWO shifts,left,right,inv,1
    EDUP
    TWO shifts,left,right,inv,0
    ret
    ENDIF
    ELSE
    DUP 7
        TWO shifts,left,right,inv,1
    EDUP
    TWO shifts,left,right,inv,0
    ret
    ENDIF
    ENDM
    ONE_CORE normal0,0,0C0h,0
    ONE_CORE normal3,2,3,0
    TWO_CORE normal1,2,0F0h,3Fh,0
    TWO_CORE normal2,4,0FCh,0Fh,0
    ONE_CORE inverse0,0,0C0h,1
    ONE_CORE inverse3,2,3,1
    TWO_CORE inverse1,2,0F0h,3Fh,1
    TWO_CORE inverse2,4,0FCh,0Fh,1
    ENDIF
    ENDMODULE
