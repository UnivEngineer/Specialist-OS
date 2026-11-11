; Нативный BANK COM: внутри интервала только штатные BIOS/MEM API.
bank_run:
    ld hl,bankSerialBulk
    ld a,(matrixFlags)
    and 64
    jp z,bankBufferReady
    ld hl,bankSerialChars
bankBufferReady:
    ld (bankRegs),hl
    ld a,(matrixKind)
    ld (bankRegs+4),a
    ld a,(matrixFlags)
    ld (bankRegs+5),a
    ld a,3
    ld (bankRegs+7),a
    ld hl,(output+1)
    ld de,kernelReal
    call bios_cmp_hl_de
    jp nz,bankRowInvoke
    ld a,4
    ld (bankRegs+7),a
bankRowInvoke:
    call bankInvoke
    ret c
    jp bankGetTime
bankSerialChars: DS 51,0
