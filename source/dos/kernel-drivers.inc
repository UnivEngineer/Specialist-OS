; Локальная таблица физического обмена. FAT не обращается к данным main BIOS.
j_diskDriver:
    jp diskDriver
    INCLUDE "../bios/blockIO.inc"

; A:/B:, B=1/2 обмен, B=3 ёмкость. При обмене BC/DE/HL сохранены.
; B: исключает физическую страницу DOS; FFh-блоки страниц остаются резервом.
diskDriver:
    ld a,b
    cp 3
    jp z,registrySize
    inc a
    jp z,registryInfo
    push bc
    push de
    push hl
    ld a,(v_drive)
    cp 7
    jp z,kernelExternalIO
    or a
    jp nz,kernelRamIO
    call dosSystemDriver
    jp kernelDiskDone
kernelExternalIO:
    call fastDriver
    jp kernelDiskDone
kernelRamIO:
    call dosRamDriver
kernelDiskDone:
    pop hl
    pop de
    pop bc
    ret
kernelSystemInfo: DS 64,0
diskDriverDummy:
    ld de,0
    ld bc,0
    ld hl,v_diskInfo
    or a
    ret
    INCLUDE "ramdisk.inc"
    INCLUDE "romproxy.inc"
