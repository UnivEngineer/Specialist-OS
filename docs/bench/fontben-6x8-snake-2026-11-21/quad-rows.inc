; Alternate the horizontal direction within one 24x8 block.
; Group entry and exit use its left edge, regardless of its vertical direction.
    MACRO STEP_COLUMN reverse
    IF reverse
    dec d
    ELSE
    inc d
    ENDIF
    ENDM

    MACRO END_HORIZONTAL_ROW
    IF !SNAKE_HORIZONTAL
    dec d
    dec d
    ENDIF
    ENDM

    MACRO STEP_ROW up
    IF up
    dec e
    ELSE
    inc e
    ENDIF
    ENDM

    MACRO RESET_ROW count,up
    ld a,e
    IF up
    add a,count
    ELSE
    sub count
    ENDIF
    ld e,a
    ENDM

    MACRO FINISH_BLOCK entry,next,limit
    inc d
    inc d
    IF QUAD_RESET_Y
    GROUP_FINISH entry,limit
    ELSE
    GROUP_FINISH next,limit
    ENDIF
    ENDM

    IF FONT_MODE == 0
    MACRO PUT_ROTATE_PIXEL inverse
    IF inverse
    cpl
    ENDIF
    ld (de),a
    ENDM

    MACRO ROTATE_RIGHT inverse,glyphA,glyphB,glyphC,glyphD
glyphB: ld l,0
    ld a,(hl)
    DUP 4
    rlca
    EDUP
    ld b,a
    and 03h
    ld c,a
glyphA: ld l,0
    ld a,(hl)
    add a,a
    add a,a
    or c
    PUT_ROTATE_PIXEL inverse
    inc d

    ld a,b
    and 0F0h
    ld c,a
glyphC: ld l,0
    ld a,(hl)
    rrca
    rrca
    ld b,a
    and 0Fh
    or c
    PUT_ROTATE_PIXEL inverse
    inc d

glyphD: ld l,0
    ld c,(hl)
    ld a,b
    and 0C0h
    or c
    PUT_ROTATE_PIXEL inverse
    ENDM

    MACRO ROTATE_LEFT inverse,glyphA,glyphB,glyphC,glyphD
glyphC: ld l,0
    ld a,(hl)
    rrca
    rrca
    ld b,a
glyphD: ld l,0
    ld c,(hl)
    and 0C0h
    or c
    PUT_ROTATE_PIXEL inverse
    dec d

    ld a,b
    and 0Fh
    ld c,a
glyphB: ld l,0
    ld a,(hl)
    DUP 4
    rlca
    EDUP
    ld b,a
    and 0F0h
    or c
    PUT_ROTATE_PIXEL inverse
    dec d

    ld a,b
    and 03h
    ld c,a
glyphA: ld l,0
    ld a,(hl)
    add a,a
    add a,a
    or c
    PUT_ROTATE_PIXEL inverse
    ENDM

    MACRO ROTATE_STEP_ROW up
    STEP_ROW up
    IF up
    dec h
    ELSE
    inc h
    ENDIF
    ENDM

    MACRO ROTATE_BLOCK entry,inverse,up,next,limit,rowPair,a0,b0,c0,d0,a1,b1,c1,d1
entry:
    call scanGroup
    ld hl,(codes)
    ld a,l
    ld (a0+1),a
    ld (a1+1),a
    ld a,h
    ld (b0+1),a
    ld (b1+1),a
    ld hl,(codes+2)
    ld a,l
    ld (c0+1),a
    ld (c1+1),a
    ld a,h
    ld (d0+1),a
    ld (d1+1),a
    ld h,FONT_ADDR/256+up*7
rowPair:
    ROTATE_RIGHT inverse,a0,b0,c0,d0
    END_HORIZONTAL_ROW
    ROTATE_STEP_ROW up
    IF SNAKE_HORIZONTAL
    ROTATE_LEFT inverse,a1,b1,c1,d1
    ELSE
    ROTATE_RIGHT inverse,a1,b1,c1,d1
    ENDIF
    END_HORIZONTAL_ROW
    ROTATE_STEP_ROW up
    ld a,h
    IF up
    cp (FONT_ADDR/256-1)&255
    ELSE
    cp FONT_ADDR/256+8
    ENDIF
    jp nz,rowPair
    IF QUAD_RESET_Y
    RESET_ROW 8,up
    ELSE
    ; The next group starts on the last raster of this group.
    STEP_ROW 1-up
    ENDIF
    FINISH_BLOCK entry,next,limit
    ENDM

    ROTATE_BLOCK groupNormalDown,0,0,groupNormalUp,normalDownLimit,ndPair,ndA0,ndB0,ndC0,ndD0,ndA1,ndB1,ndC1,ndD1
    ROTATE_BLOCK groupNormalUp,0,1,groupNormalDown,normalUpLimit,nuPair,nuA0,nuB0,nuC0,nuD0,nuA1,nuB1,nuC1,nuD1
    ROTATE_BLOCK groupInverseDown,1,0,groupInverseUp,inverseDownLimit,idPair,idA0,idB0,idC0,idD0,idA1,idB1,idC1,idD1
    ROTATE_BLOCK groupInverseUp,1,1,groupInverseDown,inverseUpLimit,iuPair,iuA0,iuB0,iuC0,iuD0,iuA1,iuB1,iuC1,iuD1
    ELSE
; FontBen attach uses 5000h for the lower planes. Each row uses the current
; format and the original two-pointer merge, with the high bytes immediate.
    ASSERT FONT_ENGINE_BASE == 02000h
SNAKE_PRE = 050h
    IF FONT_MODE == 2
P0 = SNAKE_PRE
P1 = SNAKE_PRE+24
P2 = SNAKE_PRE+32
P3 = SNAKE_PRE+8
P4 = SNAKE_PRE+16
    ELSE
P0 = SNAKE_PRE
P1 = SNAKE_PRE
P2 = SNAKE_PRE+8
P3 = SNAKE_PRE+8
P4 = FONT_ADDR/256
    ENDIF
P5 = FONT_ADDR/256

    MACRO TABLE_BYTE index,leftPage,rightPage,mask,row,inverse
    ld hl,(codes+index)
    ld c,h
    ld h,leftPage+row
    ld b,rightPage+row
    ld a,(bc)
    IF FONT_MODE == 2
    or (hl)
    ELSE
    xor (hl)
    and mask
    xor (hl)
    ENDIF
    IF inverse
    cpl
    ENDIF
    ld (de),a
    ENDM

    MACRO TABLE_ROW row,reverse,inverse
    IF reverse
    TABLE_BYTE 2,P4,P5,3Fh,row,inverse
    ELSE
    TABLE_BYTE 0,P0,P1,03h,row,inverse
    ENDIF
    STEP_COLUMN reverse
    TABLE_BYTE 1,P2,P3,0Fh,row,inverse
    STEP_COLUMN reverse
    IF reverse
    TABLE_BYTE 0,P0,P1,03h,row,inverse
    ELSE
    TABLE_BYTE 2,P4,P5,3Fh,row,inverse
    ENDIF
    END_HORIZONTAL_ROW
    ENDM

    MACRO TABLE_PAIR step,up,inverse
    TABLE_ROW step+up*(7-2*step),0,inverse
    STEP_ROW up
    TABLE_ROW step+1+up*(5-2*step),SNAKE_HORIZONTAL,inverse
    ENDM

    MACRO SNAKE_BLOCK entry,inverse,up,next,limit
entry:
    call scanGroup
    TABLE_PAIR 0,up,inverse
    STEP_ROW up
    TABLE_PAIR 2,up,inverse
    STEP_ROW up
    TABLE_PAIR 4,up,inverse
    STEP_ROW up
    TABLE_PAIR 6,up,inverse
    IF QUAD_RESET_Y
    RESET_ROW 7,up
    ENDIF
    FINISH_BLOCK entry,next,limit
    ENDM
    SNAKE_BLOCK groupNormalDown,0,0,groupNormalUp,normalDownLimit
    SNAKE_BLOCK groupNormalUp,0,1,groupNormalDown,normalUpLimit
    SNAKE_BLOCK groupInverseDown,1,0,groupInverseUp,inverseDownLimit
    SNAKE_BLOCK groupInverseUp,1,1,groupInverseDown,inverseUpLimit
    ENDIF
