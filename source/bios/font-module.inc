; Векторы Монитора-2 сохраняют перехват вывода независимо от профиля C800h.
printChar            = mon2_printCharOld
printCharRaw         = FONT_ENGINE_RAW
printCharReal        = FONT_ENGINE_RAW
printString          = mon2_printStringOld
printStringZ         = mon2_printStringOld
printStringRawZ      = FONT_ENGINE_RAWZ
printStringReal      = FONT_ENGINE_RAWZ
printStringRawLim    = FONT_ENGINE_LIM
printStringRawPseudo = FONT_ENGINE_PSEUDO
; Tab remains a control operation in the BIOS interpreter.
printChar_c9:
    ; The next stop is a multiple of four cells, measured in 2px units.
    ld a,(FONT_ENGINE_HEADER+5)
    add a,a
    ld c,a
    xor a
printCharTabNext:
    add a,c
    cp 192
    jp nc,printChar_c0A
    cp h
    jp z,printCharTabNext
    jp c,printCharTabNext
    jp saveVCursorHA
