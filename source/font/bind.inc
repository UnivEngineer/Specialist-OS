    IFNDEF FONT_BENCH_MAIN
; Publish the renderer implementations at the public BIOS JMP entries.
    ld hl,printChar
    ld (bios_printCharOld+1),hl
    ld (bios_printChar+1),hl
    ld hl,printCharRaw
    ld (bios_printCharRaw+1),hl
    ld hl,printStringZ
    ld (bios_printStringOld+1),hl
    ld (bios_printStringZ+1),hl
    ld hl,printStringRawZ
    ld (bios_printStringRawZ+1),hl
    ld hl,printStringRawLim
    ld (bios_printStringRawLim+1),hl
    ld hl,setEncoding
    ld (bios_setFontEncoding+1),hl
    ENDIF
