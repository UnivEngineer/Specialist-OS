; BC is the screen and L is the seven-bit glyph code. Each lookup
; returns the positioned fragment with zeroes outside its pixel mask.
    MACRO SCALAR6_LOOKUP ink,inv
    ld a,(bc)
    IFDEF K7_SCALAR_XOR
    xor (hl)
    ELSE
    and (255-ink)
    or (hl)
    ENDIF
    IF inv
    xor ink
    ENDIF
    ld (bc),a
    ENDM

    MACRO SCALAR6_COLUMN page,ink,inv,up
    ld h,page+7*up
    DUP 7
        SCALAR6_LOOKUP ink,inv
        IF up
        dec h
        dec bc
        ELSE
        inc h
        inc bc
        ENDIF
    EDUP
    SCALAR6_LOOKUP ink,inv
    ENDM

    MACRO SCALAR6_UPPER_HALF
    ld a,l
    xor 80h
    ld l,a
    ENDM

    MACRO SCALAR6_FULL label,page,ink,upper,inv
label:
    IF upper
    SCALAR6_UPPER_HALF
    ENDIF
    SCALAR6_COLUMN page,ink,inv,0
    ret
    ENDM

    MACRO SCALAR6_SPLIT label,left,right,leftInk,rightInk,upper,inv
label:
    IF upper
    SCALAR6_UPPER_HALF
    ENDIF
    SCALAR6_COLUMN left,leftInk,inv,0
    inc b
    SCALAR6_COLUMN right,rightInk,inv,1
    ret
    ENDM

; The dispatch uses the next X/2, three units past the glyph origin.
    SCALAR6_FULL normal0,K7_RAW/256,3Fh,1,0
    SCALAR6_FULL normal3,K7_LOWER/256,0FCh,0,0
    SCALAR6_SPLIT normal1,K7_LOWER/256+8,K7_RAW/256,0Fh,0C0h,0,0
    SCALAR6_SPLIT normal2,K7_LOWER/256,K7_LOWER/256+8,03h,0F0h,1,0
    SCALAR6_FULL inverse0,K7_RAW/256,3Fh,1,1
    SCALAR6_FULL inverse3,K7_LOWER/256,0FCh,0,1
    SCALAR6_SPLIT inverse1,K7_LOWER/256+8,K7_RAW/256,0Fh,0C0h,0,1
    SCALAR6_SPLIT inverse2,K7_LOWER/256,K7_LOWER/256+8,03h,0F0h,1,1
