; Internal BIOS callers use fixed module entries. Only public BIOS jumps are patched.
printChar = FONT_ENGINE_CHAR
printCharRaw = FONT_ENGINE_RAW
printCharReal = FONT_ENGINE_RAW
printString = FONT_ENGINE_Z
printStringZ = FONT_ENGINE_Z
printStringRawZ = FONT_ENGINE_RAWZ
printStringReal = FONT_ENGINE_RAWZ
printStringRawLim = FONT_ENGINE_LIM
; Tab remains a control operation in the BIOS interpreter.
printChar_c9:
    ; Four cells in half-pixel cursor units, for all module widths.
    ld a,(FONT_ENGINE_HEADER+5)
    add a,a
    ld c,a
    ld a,(bios_vars.cursorX)
    add a,c
    ld (bios_vars.cursorX),a
    ret
