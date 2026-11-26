; При прежнем FNT или перехваченном C809h каждый код идёт в printChar.
bbPrintStringLimLegacy:
    push af
    push bc
bbPrintStringLimLoop:
    ld a,b
    or c
    jp z,bbPrintStringLimDone
    push bc
    ld c,(hl)
    call bios_printChar
    pop bc
    inc hl
    dec bc
    jp bbPrintStringLimLoop
bbPrintStringLimDone:
    pop bc
    pop af
    ret
