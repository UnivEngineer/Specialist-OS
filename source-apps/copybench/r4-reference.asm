; Полностью автономный снимок старого FAR/R4, без текущего транспорта MEM.
    INCLUDE "r4-abi.inc"
    ORG 0FC00h
bankPage: DB 0
bankPageCount: DB 16
bankCopyBytes:
    ld a,(hl)
    ld (de),a
    inc hl
    inc de
    dec b
    jp nz,bankCopyBytes
    ret
    INCLUDE "r4-far.inc"
    ASSERT $ <= 0FF00h
    DS 0FF00h-$,0
    INCLUDE "r4-tail.inc"
    ASSERT $ <= 0FFC0h
    END
