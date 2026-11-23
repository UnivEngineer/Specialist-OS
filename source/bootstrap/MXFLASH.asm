; Драйвер загрузочного FAT12-тома во внешней flash. Исполняется в банке DOS.
    INCLUDE "../include/mxos.inc"
    ORG DOS_FLOPPY_ADDR
    ld a,b
    cp 4
    ret z
    cp 2
    jp nz,writeProtected
    ld a,d
    cp 5
    jp c,mxFlashRead
    jp nz,badBlock
    ld a,e
    cp 0A0h
    jp c,mxFlashRead
badBlock:
    ld a,DOS_ERR_ARGUMENT
    scf
    ret
writeProtected:
    ld a,DOS_ERR_WRITE_PROTECTED
    scf
    ret
    INCLUDE "mx-flash-read.inc"
    ASSERT $ <= DOS_BANK_DRIVER_ADDR
