; Read-only disk in the original MX linear system ROM (0000h..7FFFh/BFFFh).
    INCLUDE "../common/system.inc"
    INCLUDE "../../bin/banked/transport-imports.inc"
IMAGE_FLAGS = IMAGE_FLAG_DRIVER
    IFNDEF DRIVER_ORIGIN
DRIVER_ORIGIN = 0
    ENDIF
    ORG DRIVER_ORIGIN
    RELOCATE_START
driverBody:
    ex de,hl
    ld c,EXTERNAL_DRIVE
    call driverLetter
    ret c
    ld (registration+DRIVER_INSTALL.drive),a
    call argNext
    jp nc,driverLetterBad

    ; Read block zero into DOS without disturbing an installed service.
    ld hl,probeRequest
    call bankCopySector
    jp c,installNoMedia
    ld hl,boot+3
    ld de,oem
    ld b,8
.compare:
    ld a,(de)
    cp (hl)
    jp nz,installNoMedia
    inc hl
    inc de
    dec b
    jp nz,.compare
    ld hl,(boot+11)
    ld a,h
    dec a
    or l
    jp nz,installNoMedia
    ld hl,(boot+19)
    ld a,h
    or a
    jp nz,installNoMedia
    ld a,l
    cp 128
    jp z,volumeReady
    cp 192
    jp nz,installNoMedia
volumeReady:
    ld (registration+DRIVER_INSTALL.blocks),hl
    ld (serviceImage+limit+1-LOCAL_DRIVER_ADDR),a
installDriver:
    ld hl,registration
    MXOS_CALL_REGS MXOS_DOS_DRIVER_INSTALL
    ret
    INCLUDE "no-media.inc"
    INCLUDE "../common/argNext.inc"
    INCLUDE "../common/driver-letter.inc"
registration:
    DB DRIVER_INSTALL_VERSION,EXTERNAL_DRIVE,0,8
    DW 0,0
    DB DISK_CAP_READ,DISK_TECH_MX_SYSTEM_ROM
    DW serviceImage,serviceEnd-serviceImage,description
description: DB "ROM48KS",0,"MX LINEAR ROM",0
oem: DB "MXOS256 "
probeRequest: DB 080h
    DW 0
    DB DOS_BANK_PAGE
    DW boot,256

; Executed in MAIN, DOS or EXEC. CPM32 switches the ROM and destination
; independently, returning to the caller's page. LBA bit 7 is address A15.
serviceImage:
    RELOCATE_END
    DISP LOCAL_DRIVER_ADDR
    ld a,b
    cp 4
    ret z                    ; flush on a read-only medium (CF clear)
    cp 2
    jp z,readBlock
    ld a,DOS_ERR_WRITE_PROTECTED
    scf
    ret
readBlock:
    ld a,d
    or a
    jp nz,outOfRange
    ld a,e
limit:
    cp 0
    jp nc,outOfRange
    push hl
    ld (request+2),a
    ld (request+4),hl
    ld a,(bankPage)
    ld (request+3),a
    ld hl,request
    call bankCopySector
    pop hl
    ret
outOfRange:
    ld a,DOS_ERR_ARGUMENT
    scf
    ret
request: DB 080h
    DW 0
    DB 0
    DW 0,256
    ASSERT $ <= LOCAL_DRIVER_END
    ENT
    RELOCATE_START
serviceEnd:
boot: DS 256,0
    RELOCATE_END
IMAGE_BODY_SIZE  = $-driverBody
IMAGE_MEM_BEGIN  = 0
IMAGE_MEM_END    = IMAGE_BODY_SIZE
IMAGE_BSS_SIZE   = 0
IMAGE_RELOC_KIND = 0
IMAGE_ALIGN_LOG2 = 0
    RELOCATE_TABLE driverBody
    END
