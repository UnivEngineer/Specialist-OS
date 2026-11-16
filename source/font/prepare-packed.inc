; Disposable Init6.dll generator, never included in the resident slot.
; The embedded P2 has the original six bits in its low six positions.
; All I/O/validation/reservation finished before we overwrite old planes.
prepareLower:
    ld hl,(lowerBase)
    ld a,h
    or l
    ret z
    ex de,hl
    call prepareSource
    push hl
    ld bc,2048
    add hl,bc
    ld (prepareEnd+1),hl
    pop hl
prepareByte:
    ld a,(hl)
    and 3Fh
    ld b,a
    DUP 4
        rrca
    EDUP
    ld c,a ; rotated source shared by both planes
    and 3
    ld (prepareHigh+1),a
    ld a,b
    add a,a
    add a,a
prepareHigh: or 0
    ld (de),a
    ld a,d
    add a,8
    ld d,a
    ld a,c
    and 0F0h
    ld c,a
    ld a,b
    rrca
    rrca
    and 0Fh
    or c
    ld (de),a
    ld a,d
    sub 8
    ld d,a
    inc de
    inc hl
    push de
prepareEnd: ld de,0
    call bios_cmp_hl_de
    pop de
    jp nz,prepareByte
    ret
