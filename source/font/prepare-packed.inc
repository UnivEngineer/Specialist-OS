; FIN1 initializer only. P2 embeds the original six bits in its low bits.
; Expand each phase in its own pass so both pointers walk whole pages.
prepareLower:
    ld hl,(lowerBase)
    ld a,h
    or l
    ret z
    push hl
    call buildPackedLookup
    pop de
    ld b,high packedP3
    call preparePackedPhase
    ld b,high packedP1

preparePackedPhase:
    call prepareSource
    ld a,8
preparePackedPage:
    push af
preparePackedGroup:
    DUP 8
        ld c,(hl)
        ld a,(bc)
        ld (de),a
        inc e
        inc l
    EDUP
    jp nz,preparePackedGroup
    inc h
    inc d
    pop af
    dec a
    jp nz,preparePackedPage
    ret

; Build one page per phase at the end of the transient initializer workspace.
; Every raw source byte can index these pages directly, including imported P2.
packedP3 = FONT_INIT_END-512
packedP1 = FONT_INIT_END-256
buildPackedLookup:
    ld hl,packedP3
buildPackedEntry:
    ld a,l
    and 3Fh
    ld c,a
    DUP 4
        rrca
    EDUP
    ld b,a
    and 3
    ld d,a
    ld a,c
    add a,a
    add a,a
    or d
    ld (hl),a
    inc h
    ld a,c
    rrca
    rrca
    and 0Fh
    ld d,a
    ld a,b
    and 0F0h
    or d
    ld (hl),a
    dec h
    inc l
    jp nz,buildPackedEntry
    ret
