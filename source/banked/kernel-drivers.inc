; Локальная таблица физического обмена. FAT не обращается к данным main BIOS.
j_diskDriver:
    jp diskDriver
    INCLUDE "../bios/blockIO.inc"

; A:/B:, B=1/2 обмен, B=3 ёмкость. При обмене BC/DE/HL сохранены.
; B: исключает физическую страницу DOS; FFh-блоки страниц остаются резервом.
diskDriver:
    ld a,b
    cp 3
    jp z,kernelDiskSize
    inc a
    jp z,kernelDiskInfo
    push bc
    push de
    push hl
    ld a,(v_drive)
    or a
    jp nz,kernelRamIO
    call dosSystemDriver
    jp kernelDiskDone
kernelRamIO:
    call dosRamDriver
kernelDiskDone:
    pop hl
    pop de
    pop bc
    ret
kernelDiskSize:
    ld a,(v_drive)
    or a
    jp nz,kernelRamSize
    ld hl,kernelSystemInfo
    call dosSystemDriver
    ld hl,v_diskInfo
    ret
kernelRamSize:
    ld a,(bankPageCount)
    dec a
    ld h,a
    ld l,0
kernelDiskSizeReady:
    ex de,hl
    ld hl,v_diskInfo
    ld bc,0
    or a
    ret
kernelDiskInfo:
    ld a,(v_drive)
    or a
    jp nz,kernelRamInfo
    ld hl,kernelSystemInfo
    jp dosSystemDriver
kernelRamInfo:
    ld bc,(DISK_TECH_RAM << 8)+DISK_CAP_READ+DISK_CAP_WRITE+DISK_CAP_VERIFY
    ld hl,kernelDiskName
    or a
    ret
kernelDiskName: DB "Bank RAM",0,"DOS page reserved",0
kernelSystemInfo: DS 64,0
diskDriverDummy:
    ld de,0
    ld bc,0
    ld hl,v_diskInfo
    or a
    ret
    INCLUDE "ramdisk.inc"
    INCLUDE "romproxy.inc"
