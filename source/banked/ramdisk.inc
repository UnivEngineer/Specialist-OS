; Блочный драйвер B: в странице DOS. Первую физическую страницу занимает DOS.
; Как прежде, каждый 256-й блок (хвост страницы) недоступен носителю.
; B=1/2, DE=логический блок, HL=буфер DOS, CF/A=ошибка. BC/DE/HL разрушены.
; Страница DOS логически равна 1; данные RAM-диска начинаются с FAR=2.
dosRamDriver:
    ld a,e
    cp 0FFh
    jp z,dosRamBad
    ld a,(bankPageCount)
    cp 2
    jp c,dosRamBad
    dec a
    cp d
    jp c,dosRamBad
    jp z,dosRamBad
    ld a,d
    add a,2
    ld d,e
    ld e,0
    dec b
    jp z,dosRamWrite
    dec b
    jp nz,dosRamBad
    ld (dosRamCopy),a
    ld (dosRamCopy+4),hl
    ex de,hl
    ld (dosRamCopy+1),hl
    ld a,DOS_IO_PAGE
    ld (dosRamCopy+3),a
    jp dosRamRun
dosRamWrite:
    ld (dosRamCopy+3),a
    ld (dosRamCopy+1),hl
    ex de,hl
    ld (dosRamCopy+4),hl
    ld a,DOS_IO_PAGE
    ld (dosRamCopy),a
dosRamRun:
    ld hl,dosRamCopy
    call bankCopyFar
    ret nc
dosRamBad:
    ld a,DOS_ERR_ARGUMENT
    scf
    ret
dosRamCopy: DB 0
    DW 0
    DB 0
    DW 0,256

