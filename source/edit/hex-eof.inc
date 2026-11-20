hexReserveHeader:
    dec b
    ld a,c
    add a,10
    ld c,a
    ret

hexMakeEOF:
    call hexBlankRow
    ld hl,(hexSize)
    call hexLoadAddress
    HEX_ROW_TARGET
    jp hexWord
