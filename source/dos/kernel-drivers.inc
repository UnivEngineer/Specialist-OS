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
    call dosPhysicalDrive
    cp FLOPPY_DRIVE
    jp z,kernelFloppyIO
    cp EXTERNAL_DRIVE
    jp z,kernelExternalIO
    cp SYSTEM_DRIVE
    jp nz,kernelRamIO
    call dosSystemDriver
    jp kernelDiskDone
kernelFloppyIO:
    call DOS_FLOPPY_ADDR
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
    ret nc
    cp DOS_ERR_NO_MEDIA
    jp z,dosMediaMissing
    scf
    ret
diskDriverDummy:
    ld de,0
    ld bc,0
    ld hl,v_diskInfo
    or a
    ret
; Current DOS letter -> stable local transport selector in A.
; AF changes, BC/DE/HL stay intact. C: and B: use their own indices.
dosPhysicalDrive:
    push hl
    ld a,(v_drive)
    ld hl,kernelRamDrive
    cp (hl)
    jp nz,dosPhysicalExternal
    ld a,RAM_DRIVE
    jp dosPhysicalDone
dosPhysicalExternal:
    ld hl,kernelExternalDrive
    cp (hl)
    jp nz,dosPhysicalFloppy
    ld a,EXTERNAL_DRIVE
    pop hl
    ret
dosPhysicalFloppy:
    dec hl
    cp (hl)
    jp z,dosPhysicalIsFloppy
    cp SYSTEM_DRIVE
    jp z,dosPhysicalDone
    ld a,0FFh
    jp dosPhysicalDone
dosPhysicalIsFloppy:
    ld a,FLOPPY_DRIVE
dosPhysicalDone:
    pop hl
    ret
    INCLUDE "ramdisk.inc"
    INCLUDE "romproxy.inc"
