; All four phases are present across the three packed tables. Each byte
; has a second fragment outside ink, masked out during the screen merge.
    MACRO PACKED_SCALAR_ROW ink,inv
    IFDEF K7_SCALAR_XOR
    ld a,(hl)
    and ink
    ld d,a
    ld a,(bc)
    xor d
    ELSE
    ld a,(bc)
    xor (hl)
    and (255-ink)
    xor (hl)
    ENDIF
    IF inv
    xor ink
    ENDIF
    ld (bc),a
    ENDM

    MACRO PACKED_SCALAR_HALF ink,inv,up
    DUP 4
        IF up
        dec h
        dec bc
        ENDIF
        PACKED_SCALAR_ROW ink,inv
        IF !up
        inc h
        inc bc
        ENDIF
    EDUP
    ret
    ENDM

    MACRO PACKED_SCALAR_TAG
    ld a,l
    xor 80h
    ld l,a
    ENDM

    MACRO PACKED_SCALAR_FULL label,page,ink,inv
label:
    ld h,page
    call .half
    PACKED_SCALAR_TAG
    ld h,page
.half:
    PACKED_SCALAR_HALF ink,inv,0
    ENDM

    MACRO PACKED_SCALAR_SPLIT label,left,right,leftInk,rightInk,inv
label:
    ld h,left
    call .downHalf
    PACKED_SCALAR_TAG
    ld h,left
    call .downHalf
    inc b
    ld h,right+4
    call .upHalf
    PACKED_SCALAR_TAG
    ld h,right+4
.upHalf:
    PACKED_SCALAR_HALF rightInk,inv,1
.downHalf:
    PACKED_SCALAR_HALF leftInk,inv,0
    ENDM

; The dispatch uses the next X/2. Table pages contain shifts 2, 4 and 6.
    PACKED_SCALAR_FULL normal0,K7_RAW/256,3Fh,0
    PACKED_SCALAR_FULL normal3,K7_TABLES/256,0FCh,0
    PACKED_SCALAR_SPLIT normal1,K7_TABLES/256+4,K7_RAW/256,0Fh,0C0h,0
    PACKED_SCALAR_SPLIT normal2,K7_TABLES/256,K7_TABLES/256+4,03h,0F0h,0
    PACKED_SCALAR_FULL inverse0,K7_RAW/256,3Fh,1
    PACKED_SCALAR_FULL inverse3,K7_TABLES/256,0FCh,1
    PACKED_SCALAR_SPLIT inverse1,K7_TABLES/256+4,K7_RAW/256,0Fh,0C0h,1
    PACKED_SCALAR_SPLIT inverse2,K7_TABLES/256,K7_TABLES/256+4,03h,0F0h,1
