; Page order is phase (2,4,6 pixels), then row. In the full format
; the head and spill pages of each row are adjacent for INC H/DEC H.
preparePacked8:
    ld hl,(lowerBase)
    ex de,hl
    call packedShift2
    call packedShift4
    call packedShift6
    xor a
    ret
prepareFull8:
    ld hl,(lowerBase)
    ex de,hl
    call fullShift2
    call fullShift4
    call fullShift6
    xor a
    ret

    MACRO PREPARE_SHIFT label,shift,full
label:
    ld hl,(source)
    ld b,8
.row:
    ld a,(hl)
    IF shift == 6
    rlca
    rlca
    ELSE
    DUP shift
        rrca
    EDUP
    ENDIF
    IF full
    ld c,a
    and 255>>shift
    ld (de),a
    inc d
    ld a,c
    and 256-(256>>shift)
    ld (de),a
    dec d
    ELSE
    ld (de),a
    ENDIF
    inc l
    inc e
    jp nz,.row
    inc h
    inc d
    IF full
    inc d
    ENDIF
    dec b
    jp nz,.row
    ret
    ENDM

    PREPARE_SHIFT packedShift2,2,0
    PREPARE_SHIFT packedShift4,4,0
    PREPARE_SHIFT packedShift6,6,0
    PREPARE_SHIFT fullShift2,2,1
    PREPARE_SHIFT fullShift4,4,1
    PREPARE_SHIFT fullShift6,6,1
