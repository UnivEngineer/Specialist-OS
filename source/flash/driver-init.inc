; Инициализаторы используют одну посадочную область. После RET всё тело
; можно затереть: реестр DOS владеет строками и копией подготовленной службы.
    INCLUDE "../../bin/banked/flash-imports.inc"
    ORG DOS_BANK_DRIVER_ADDR
    IF LOCAL_KIND != 0
    call initProbe.flashDosInit
    ret c
    ld a,(initProbe.flashChipCount)
    add a,a
    add a,a
    add a,a
    ld (registration+DRIVER_INSTALL.blocks+1),a
    ENDIF
    ld hl,registration
    MXCALL MX_DOS_DRIVERINSTALL
    ret
registration:
    DB DRIVER_INSTALL_VERSION,7,LOCAL_KIND,8
    IF LOCAL_KIND == 0
    DW 256,0
    DB DISK_CAP_READ,DISK_TECH_ROM
    ELSE
        IF LOCAL_KIND == 1
    DW 0,0
    DB DISK_CAP_READ+DISK_CAP_WRITE+DISK_CAP_VERIFY,DISK_TECH_FLASH
        ELSE
    DW 0,0800h
    DB DISK_CAP_READ+DISK_CAP_WRITE+DISK_CAP_VERIFY+DISK_CAP_FLUSH,DISK_TECH_FLASH
        ENDIF
    ENDIF
    DW serviceImage,serviceEnd-serviceImage,description
serviceImage:
    IF LOCAL_KIND == 0
    INCBIN "../../bin/banked/LOCALDRV0.bin"
    ELSE
        IF LOCAL_KIND == 1
    INCBIN "../../bin/banked/LOCALDRV1.bin"
        ELSE
    INCBIN "../../bin/banked/LOCALDRV2.bin"
        ENDIF
    ENDIF
serviceEnd:
description:
    IF LOCAL_KIND == 0
    DB "ROM",0,"64K read only",0
    ELSE
        IF LOCAL_KIND == 1
    DB "FLASH",0
        ELSE
    DB "FLASHFST",0
        ENDIF
chipDescription: DB "0x512K ",0
    MODULE initProbe
    IF LOCAL_KIND == 1
SectorLimit = serviceImage+local1SectorLimit
v_chipMap = serviceImage+local1v_chipMap
    ELSE
SectorLimit = serviceImage+local2SectorLimit
v_chipMap = serviceImage+local2v_chipMap
    ENDIF
DriverDescription = chipDescription
; Старый стенд ожидает патч FuncSize+2, здесь он попадает в дескриптор.
FuncSize = registration+DRIVER_INSTALL.blocks-1
LATCH_0 = 00Ah
LATCH_1 = 00Bh
WRITE_0 = 00Ch
WRITE_1 = 00Dh
MASK_STANDBY = 0C0h
MASK_READ = 040h
PPI_MODE_WRITE = 080h
PPI_MODE_READ = 090h
    INCLUDE "../bootstrap/tests/flash-init.inc"
    ENDMODULE
    ENDIF
    ASSERT $ <= DOS_BANK_DRIVER_END
