attach:
    ld hl,K7_RAW
    ld (bios_upper.charGen),hl
    ld a,0FFh
    ld (bios_upper.koi7),a
    INCLUDE "bind.inc"
    xor a
setEncoding:
    ret
service:
    cp FONT_RENDER_DISABLE+1
    jp nc,unsupported
    ld hl,0
    ld de,K7_RAW
    ld bc,0
    xor a
    ret
unsupported:
    ld a,DOS_ERR_UNSUPPORTED
    scf
    ret
    INCLUDE "bench.inc"
