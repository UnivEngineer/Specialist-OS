mainIoAddress:
    ld hl,(mainIoRequest+3)
    ld (io_vars.romSource),hl
    ld hl,(mainIoRequest+1)
    ld a,l
    add a,a
    ld a,h
    rla
    ld (io_vars.romPage),a
    ld a,l
    and 07Fh
    ld h,a
    ld l,0
    ld (io_vars.romTarget),hl
    ret

