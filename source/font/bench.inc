; Optional FontBen MAIN module wrapper. Pixel kernels and public frontends
; are compiled verbatim from the production source; only installation differs.
    IFDEF FONT_BENCH_MAIN
benchInitialize:
    IFDEF FONT_BENCH_STREAM
    ; FontBen passes the stream selector in A bit 7.
    and 80h
    IFDEF FONT4_COMPACT
    ld (benchStream),a
    ENDIF
    ld hl,fast6.tryGroup
    jp z,benchSetGroup
    ld hl,rowStream.tryGroup
benchSetGroup:
    ld (stringTryGroup+1),hl
    ENDIF
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
    IFDEF FONT_BENCH_STREAM
stringTryGroup:
    jp fast6.tryGroup
    IFDEF FONT4_COMPACT
benchStream: DB 0
    ENDIF
    ENDIF
; Direct frontend descriptor: width, scalar, five public entries, encoding.
benchDescriptor:
    IFDEF CELL_WIDTH
    DB CELL_WIDTH
    ELSE
    DB 6
    ENDIF
    DW scalar,printChar,printCharRaw,printStringZ,printStringRawZ,printStringRawLim,setEncoding
    ENDIF
