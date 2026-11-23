    ld hl,mxRomService
    ld de,mxRomServiceEnd
    ld bc,MX_ROM_SLOT
    call moveCore
    call MX_ROM_SLOT+3
    ld hl,MX_ROM_SLOT
    ld (MX_ROM_TABLE),hl
    ld a,'A'
    ld (MX_AUTOEXEC_PATH),a
    ld (MX_CLI_PATH),a
    ld (MX_NC_PATH),a
    ld (MX_NC_MAIN_PATH),a
    call mxCheckWarm
    jp z,dosLoaded
    ; Незавершённая холодная загрузка не должна помечать старое ядро готовым.
    ld hl,DOS_READY
    ld c,0
    call 0FFC0h
    ld e,MX_KERNEL_LBA
mxNextSector:
    ld d,0
    push de
    ld hl,0
    ld b,2
    ld a,(mxBootMedium)
    or a
    jp nz,mxReadFlash
    call fdTransfer
    jp mxReadDone
mxReadFlash:
    call mxFlashRead
mxReadDone:
    pop de
    jp c,mxRetry
    push de
    ld hl,0
    ld de,511
    call MX_CALC_CS
    pop de
    ld a,e
    sub MX_KERNEL_LBA
    ld l,a
    ld h,0
    add hl,hl
    push de
    ld de,mxChecks
    add hl,de
    pop de
    ld a,(hl)
    cp c
    jp nz,mxRetry
    inc hl
    ld a,(hl)
    cp b
    jp nz,mxRetry
    ld a,e
    sub MX_KERNEL_LBA
    add a,a
    add a,MODULE_LOAD >> 8
    ld h,a
    ld l,0
    push de
    ld de,0
    ld bc,512
    xor a
    call install
    pop de
    inc e
    ld a,e
    cp MX_KERNEL_LBA+MX_DISK_SECTORS
    jp c,mxNextSector
    ; Устанавливаем драйвер и описание загрузочного носителя A:.
    ld de,mxResident
    ld bc,mxResidentEnd-mxResident
    ld a,(mxBootMedium)
    or a
    jp z,mxInstallDriver
    ld de,mxFlashRecord
    ld hl,MX_BOOT_FLOPPY_RECORD
    ld bc,MX_BOOT_FLOPPY_RECORD_BYTES
    xor a
    call install
    ld de,mxFlashResident
    ld bc,mxFlashResidentEnd-mxFlashResident
mxInstallDriver:
    ld hl,MX_FLOPPY_ADDR
    add hl,bc
    ld (mxDriverLimit),hl
    ld hl,MX_FLOPPY_ADDR
    xor a
    call install
    ld de,mxDriverLimit
    ld hl,MX_BOOT_FLOPPY_END
    ld bc,2
    xor a
    call install
    ld de,mxBootMedium
    ld hl,MX_BOOT_COOKIE
    ld bc,mxBootIdentityEnd-mxBootMedium
    xor a
    call install
    jp dosLoaded
mxDriverLimit: DW 0
mxBootMedium:  DB 0
mxRomBundle:   DB 0
               DD MX_BOOT_ID
mxBootIdentityEnd:
    ASSERT mxBootIdentityEnd-mxBootMedium == 6
mxResident:
    INCBIN "../../bin/mx/fd-resident.bin"
mxResidentEnd:
mxFlashRecord:
    DW 1440,0
    DB DISK_CAP_READ,DISK_TECH_MX_VV55_FLASH,9,DRIVER_KIND_FLOPPY
    DB "MX FLASH",0,"VV55 FAT12",0
    ASSERT $-mxFlashRecord == MX_BOOT_FLOPPY_RECORD_BYTES
mxFlashResident:
    INCBIN "../../bin/mx/flash-resident.bin"
mxFlashResidentEnd:
mxRetry:
    ld hl,mxRecovery
    ld de,mxRecoveryEnd
    ld bc,0FFC0h
    call moveCore
    ld a,(mxBootMedium)
    or a
    jp nz,0FFC0h
    jp 0FFC6h
mxRecovery:
    ld (IO_PAGE_ROM),a
    jp 0103h
    ld (IO_PAGE_ROM),a
    jp 0106h
mxRecoveryEnd:
    INCLUDE "mx-flash-read.inc"
mxChecks:
    INCLUDE "../../bin/mx/checks.inc"
    INCLUDE "../floppy/transfer.inc"
mxRomService:
    INCBIN "../../bin/mx/MXROM.bin"
mxRomServiceEnd:
