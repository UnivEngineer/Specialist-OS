; HL points to mask. Decode HL/DE/BC/AF and optional inline HL.
    ld c,(hl)
    inc hl
    ex de,hl
    ld hl,0
    ld (BATCH_REGS),hl
    ld (BATCH_REGS+2),hl
    ld (BATCH_REGS+4),hl
    ld (BATCH_REGS+6),hl
    ld hl,BATCH_REGS
    ld b,4
unpackArguments:
    ld a,c
    rrca
    ld c,a
    jp nc,unpackSkip
    ld a,(de)
    ld (hl),a
    inc de
    inc hl
    ld a,(de)
    ld (hl),a
    inc de
    dec hl
unpackSkip:
    inc hl
    inc hl
    dec b
    jp nz,unpackArguments
    ld a,c
    and 1 ; original bit4: HL points after the packed register words
    jp z,unpackCall
    ld hl,BATCH_REGS
    ld (hl),e
    inc hl
    ld (hl),d
unpackCall:
