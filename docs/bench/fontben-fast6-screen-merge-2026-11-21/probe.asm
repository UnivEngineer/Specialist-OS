    ORG 03000h

    MACRO PART page,merge,inverse
    ld b,page
    ld a,(bc)
    IF merge
    or (hl)
    IF inverse
    cpl
    ENDIF
    ENDIF
    ld (hl),a
    IF merge
    inc h
    ENDIF
    ENDM

    MACRO DIRECT_GLYPH advance
    IF advance
    inc de
    ENDIF
    ld a,(de)
    ld c,a
    ENDM

; Each variant receives font pages for one raster, patched by the harness.
; HL points to the screen. DE (direct) or SP (stack) points to the text.
    MACRO GROUP entry,bodyEnd,limit,finish,stack,stream,inverse
entry:
    IF stack
    pop de
    ld c,e
    ELSE
    DIRECT_GLYPH 0
    ENDIF
    PART 050h,0,inverse
    IF stack
    ld c,d
    ELSE
    DIRECT_GLYPH 1
    ENDIF
    PART 068h,1,inverse
    PART 070h,0,inverse
    IF stack
    pop de
    ld c,e
    ELSE
    DIRECT_GLYPH 1
    ENDIF
    PART 058h,1,inverse
    PART 060h,0,inverse
    IF stack
    ld c,d
    ELSE
    DIRECT_GLYPH 1
    ENDIF
    PART 040h,1,inverse
bodyEnd:
    IF stream
    IF !stack
    inc de
    ENDIF
    ld a,h
limit: cp 0
    jp nz,entry
    ENDIF
finish:
    nop
    ENDM

    GROUP direct,directBodyEnd,directNoLimit,directEnd,0,0,0
    GROUP directStream,directStreamBodyEnd,directLimit,directStreamEnd,0,1,0
    GROUP directInverse,directInverseBodyEnd,directInverseLimit,directInverseEnd,0,1,1
    GROUP stackStream,stackStreamBodyEnd,stackLimit,stackStreamEnd,1,1,0
    GROUP stackInverse,stackInverseBodyEnd,stackInverseLimit,stackInverseEnd,1,1,1

    MACRO FONT_INDEX advance
    IF advance
    inc de
    ENDIF
    ld a,(de)
    ld l,a
    ENDM

; SP temporarily holds the screen address. B/C keep the first two finished
; screen bytes. The fragment of the third byte lives in H until the last
; code has been read. The caller must preserve the real stack pointer.
    MACRO PARK_SCREEN entry,bodyEnd,limit,finish,inverse
entry:
    ld sp,hl
    FONT_INDEX 0
    ld h,050h
    ld c,(hl)

    FONT_INDEX 1
    ld h,068h
    ld a,(hl)
    or c
    IF inverse
    cpl
    ENDIF
    ld b,a
    ld h,070h
    ld c,(hl)

    FONT_INDEX 1
    ld h,058h
    ld a,(hl)
    or c
    IF inverse
    cpl
    ENDIF
    ld c,a
    ld h,060h
    ld h,(hl)

    FONT_INDEX 1
    ld a,h
    ld h,040h
    or (hl)
    IF inverse
    cpl
    ENDIF

    ld hl,0
    add hl,sp
    ld (hl),b
    inc h
    ld (hl),c
    inc h
    ld (hl),a
    inc h
bodyEnd:
    inc de
    ld a,h
limit: cp 0
    jp nz,entry
finish:
    nop
    ENDM

    PARK_SCREEN parkStream,parkStreamBodyEnd,parkLimit,parkStreamEnd,0
    PARK_SCREEN parkInverse,parkInverseBodyEnd,parkInverseLimit,parkInverseEnd,1
