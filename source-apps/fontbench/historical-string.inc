; Historical ASCIZ loops for MXOS1 / Monitor-2 / RAMFOS.
; Only normal StringZ is replaced; RawZ/Lim have no historical equivalent.
; Pixel bodies use FontBen's common full font and cursor coordinates.
; The complete per-character adapter saves AF/BC/DE/HL, handles controls
; through the installed BIOS, and wraps like the other standalone kernels.
; This intentionally retains per-character saves (unlike modern batches).
; It is not an emulation of ROM-specific keyboard polling / printer echo.
engineHistoricalMode: DB 0
historicalStringSelect:
    ld hl,hookZ
    ld a,(engineHistoricalMode)
    or a
    jp z,historicalSelected
    ld hl,historicalMxosZ
    dec a
    jp z,historicalSelected
    ld hl,historicalMonitorAdapter
    dec a
    jp z,historicalSelected
    ld hl,historicalRamfosAdapter
    dec a
    jp z,historicalSelected
    ld hl,historicalWindowZ
historicalSelected:
    ld (hookTargets+8),hl
    ld (hookTargets+10),hl
    ld (hookTargets+12),hl
    ret

; Copy of MXOS-1 printString: save BC/AF once, public char for each byte.
historicalMxosZ:
    push bc
    push af
historicalMxosNext:
    ld a,(hl)
    or a
    jp z,historicalMxosDone
    ld c,a
    call historicalChar
    inc hl
    jp historicalMxosNext
historicalMxosDone:
    pop af
    pop bc
    ret

; Monitor-2 LCE1D / RAMFOS printString do not preserve all modern StringZ
; registers. A single outer adapter supplies the benchmark's public contract.
; Its constant entry/exit cost is included in all measured calls/chunks.
historicalMonitorAdapter:
    push af
    push bc
    push de
    call historicalMonitorZ
    jp historicalStringDone
historicalMonitorZ:
    ld a,(hl)
    ld c,a
    cp 0
    ret z
    call historicalChar
    inc hl
    jp historicalMonitorZ

historicalRamfosAdapter:
    push af
    push bc
    push de
    call historicalRamfosZ
historicalStringDone:
    pop de
    pop bc
    pop af
    ret
historicalRamfosZ:
    ld a,(hl)
    and a
    ret z
    call historicalPrintCharA
    inc hl
    jp historicalRamfosZ
; RAMFOS C80F printCharA, including the extra BC save around printChar.
historicalPrintCharA:
    push bc
    ld c,a
    call historicalChar
    pop bc
    ret

historicalChar:
    push af
    push bc
    push de
    push hl
    ld a,(bios_upper.escMode)
    inc a
    jp nz,historicalControl
    ld a,c
    cp 20h
    jp c,historicalControl
    call historicalGlyph
    call engineWrap
    jp historicalCharDone
historicalControl:
    call originalChar
historicalCharDone:
    pop hl
    pop de
    pop bc
    pop af
    ret
historicalGlyph: jp 0

; Window monitor CA4D-CA5E: BC/AF saved once, full character call per byte.
historicalWindowZ:
    push bc
    push af
historicalWindowNext:
    ld a,(hl)
    ld c,a
    or a
    jp z,historicalWindowDone
    call historicalChar
    inc hl
    jp historicalWindowNext
historicalWindowDone:
    pop af
    pop bc
    ret
