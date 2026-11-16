; Optional FontBen MAIN module wrapper. Pixel kernels and public frontends
; are compiled verbatim from the production source; only installation differs.
    IFDEF FONT_BENCH_MAIN
benchInitialize:
    ld hl,(bios_upper.charGen)
    push hl
    ld a,(bios_upper.koi7)
    push af
    ld de,05000h
    call attach
    pop af
    ld (bios_upper.koi7),a
    pop hl
    ld (bios_upper.charGen),hl
    ld hl,benchDescriptor
    xor a
    ret
; Direct frontend descriptor: width, scalar, five public entries, encoding.
benchDescriptor:
    IFDEF CELL_WIDTH
    DB CELL_WIDTH
    ELSE
    DB 6
    ENDIF
    DW scalar,printChar,printCharRaw,printStringZ,printStringRawZ,printStringRawLim,setEncoding
    ENDIF
