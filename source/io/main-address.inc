mainIoAddress:
    ld hl,(mainIoRequest+1)
    ld a,(io_vars.romBlocksHi)
    or a
    jp z,mainIoLinear
    ld a,l
    add a,a
    ld a,h
    rla
    ld h,a
    ld a,l
    and 07Fh
    ld l,a
    ld a,h
mainIoLinear:
    ld h,l
    ld l,0
    or 080h
    ret

