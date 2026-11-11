; All three preserved CP/M 4x8 formats, selected at initialization.
    INCLUDE "../../source/include/mxos.inc"
    DEFINE FONT_ENGINE_DLL
IMAGE_FLAGS = IMAGE_FLAG_MAIN
    ORG 02000h
entry: jp initialize
descriptor: DB 4,2
    DW font4ScalarHelper,font4Batch,empty
    DS 06000h-$,0
initialize:
    call font4Prepare
    ld hl,descriptor
    xor a
    ret
empty: ret
bench_copy:
    ld a,(hl)
    ld (de),a
    inc hl
    inc de
    dec bc
    ld a,b
    or c
    jp nz,bench_copy
    ret
font4Source: INCBIN "../../bin/FONTBEN.font4.bin"
    INCLUDE "engines/font4.inc"
    ASSERT $ <= 07C00h
end:
