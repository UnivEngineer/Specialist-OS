bbEntry:
    ld b,0
    ld hl,bbModes
    add hl,bc
    ld a,(hl)
    dec a
    jp nz,bbUnsupported

bbScalarFast:
    ld hl,bbScalarDone
    push hl
    ld hl,(MEM_NATIVE_CALL)
    ld de,6
    add hl,de
    ld de,BANK_FRAME_REGS
    jp (hl)
bbScalarDone:
    push af
    xor a
    ld (BANK_FRAME_STATUS),a
    pop af
    ret

bbFetch:
    ld (bbCopy+1),hl
    ex de,hl
    ld (bbCopy+4),hl
    ld h,b
    ld l,c
    ld (bbCopy+6),hl
    ld a,(bbPage)
    ld (bbCopy),a
    xor a
    ld (bbCopy+3),a
    ld hl,bbCopy
    jp bankCopyFar

bbUnsupported:
    ld a,DOS_ERR_UNSUPPORTED
    scf
    ret
