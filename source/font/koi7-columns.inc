; DE is the screen, L/C are two glyph codes. Split tables place rasters
; y and y+4 in the two halves of each page.
    MACRO COLUMN_STEP up
    IF up
    dec b
    dec h
    dec e
    ELSE
    inc b
    inc h
    inc e
    ENDIF
    ENDM
    IF K7_MODE == 0 || K7_MODE == 2
    INCLUDE "rotate-columns.inc"
    ENDIF
    MACRO TAG_ROWS operation
    ld a,l
    operation 80h
    ld l,a
    ld a,c
    operation 80h
    ld c,a
    ENDM

    MACRO SPLIT_RASTERS offset,mask,inverse,up,count
    DUP count
    IF up
    COLUMN_STEP up
    ENDIF
    IF K7_MODE == 1
    ld a,(bc)
    xor (hl)
    and mask
    xor (hl)
    IF inverse
    cpl
    ENDIF
    ld (de),a
    ELSE
    ROTATE_COLUMN_BYTE offset,inverse
    ENDIF
    IF !up
    COLUMN_STEP up
    ENDIF
    EDUP
    ENDM

    MACRO SPLIT_COLUMN offset,page,mask,inverse,up
    ld hl,(codes+offset)
    IF K7_MODE != 1 && offset != 0
    ld c,l
    ld l,h
    ELSE
    ld c,h
    ENDIF
    IF up
    TAG_ROWS or
    ENDIF
    IF K7_MODE == 0
    ld h,page+(up)*4
    ld b,h
    SPLIT_RASTERS offset,mask,inverse,up,4
    TAG_ROWS xor
    ld h,page+(up)*4
    ld b,h
    SPLIT_RASTERS offset,mask,inverse,up,4
    ELSE
.half:
    ld h,page+(up)*4
    ld b,h
.raster:
    IF K7_MODE == 1 || (K7_MODE == 2 && offset == 2)
    SPLIT_RASTERS offset,mask,inverse,up,4
    ELSE
    SPLIT_RASTERS offset,mask,inverse,up,2
    ld a,h
    cp page+4*(1-(up))
    jp nz,.raster
    ENDIF
    ld a,l
    xor 80h
    ld l,a
    IF up
    jp m,.done
    ELSE
    jp p,.done
    ENDIF
    ld a,c
    xor 80h
    ld c,a
    jp .half
.done:
    ENDIF
    ENDM

    MACRO OR_RASTER inverse
    ld a,(bc)
    or (hl)
    IF inverse
    cpl
    ENDIF
    ld (de),a
    ENDM

    MACRO SEPARATE_COLUMN offset,page,inverse,up
    ld hl,(codes+offset)
    ld c,h
    IF K7_MODE == 2
    ld a,l
    or 80h
    ld l,a
    ENDIF
    IF K7_MODE == 3
    ld h,page+(up)*7
    ld b,h
    DUP 7
    OR_RASTER inverse
    COLUMN_STEP up
    EDUP
    OR_RASTER inverse
    ELSE
    ld h,page+(up)*8
    ld b,h
.rows:
    DUP 4
    IF up
    COLUMN_STEP up
    ENDIF
    OR_RASTER inverse
    IF !up
    COLUMN_STEP up
    ENDIF
    EDUP
    ld a,h
    cp page+8*(1-(up))
    jp nz,.rows
    ENDIF
    ENDM

    MACRO KOI7_BLOCK entry,inverse,up,next,limit
entry:
    call scanGroup
    IF K7_MODE == 3
    SEPARATE_COLUMN 0,K7_LOWER/256,inverse,up
    inc d
    SEPARATE_COLUMN 1,K7_LOWER/256+8,inverse,1-up
    inc d
    SEPARATE_COLUMN 2,K7_RAW/256,inverse,up
    ELSE
    IF K7_MODE == 1
    SPLIT_COLUMN 0,K7_TABLES/256,03h,inverse,up
    ELSE
    SPLIT_COLUMN 0,K7_RAW/256,03h,inverse,up
    ENDIF
    inc d
    IF K7_MODE == 2
    SEPARATE_COLUMN 1,K7_TABLES/256,inverse,1-up
    ELSE
    IF K7_MODE == 1
    SPLIT_COLUMN 1,K7_TABLES/256+4,0Fh,inverse,1-up
    ELSE
    SPLIT_COLUMN 1,K7_RAW/256,0Fh,inverse,1-up
    ENDIF
    ENDIF
    inc d
    SPLIT_COLUMN 2,K7_RAW/256,3Fh,inverse,up
    ENDIF
    GROUP_FINISH next,limit
    ENDM

    KOI7_BLOCK groupNormalDown,0,0,groupNormalUp,normalDownLimit
    KOI7_BLOCK groupNormalUp,0,1,groupNormalDown,normalUpLimit
    KOI7_BLOCK groupInverseDown,1,0,groupInverseUp,inverseDownLimit
    KOI7_BLOCK groupInverseUp,1,1,groupInverseDown,inverseUpLimit
