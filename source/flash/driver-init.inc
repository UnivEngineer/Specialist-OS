; Инициализаторы используют одну посадочную область. После RET всё тело
; можно затереть: реестр DOS владеет строками и копией подготовленной службы.
    INCLUDE "../../bin/banked/flash-imports.inc"
    ORG DOS_BANK_DRIVER_ADDR
    IFNDEF ROM_LEGACY
    DEFINE ROM_LEGACY 0
    ENDIF
    IF LOCAL_KIND == 0
    call initRom.detect
    ret c
    ld (registration+DRIVER_INSTALL.blocks),hl
    ENDIF
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
    MXOS_CALL MXOS_DOS_DRIVER_INSTALL
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
        IF ROM_LEGACY
    DB "ROM64K",0,"64K DIRECT PPI",0
        ELSE
    DB "ROM2M",0,"PIO READ ONLY",0
        ENDIF
; Опрос использует отдельную нерезидентную копию: прежний слот H:
; не затрагивается до успешной проверки BPB и регистрации в DOS.
    MODULE initRom
detect:
    MXOS_CALL MXOS_DOS_FLUSH_VOLUME
    ret c
        IF !ROM_LEGACY
    ld hl,FlashRead
    ld (RomReadSelect+1),hl
        ENDIF
    ld hl,boot
    ld de,0
    ld b,2
    call Driver
    ld hl,boot+256
    ld de,1
    ld b,2
    call Driver
    call capacity
    ret c
        IF ROM_LEGACY
    ld a,h
    cp 1
    jp c,good
    jp nz,bad
    ld a,l
    or a
    ret z
    jp bad
        ELSE
    push hl
    ld hl,LOCAL_DRIVER_ADDR+local0FlashRead
    ld (serviceImage+local0RomReadSelect+1),hl
    pop hl
    ret
        ENDIF
; BPB: 256/512 байт на логический сектор, до 8192 физических блоков.
; HL=блоки, CF/A=ошибка. AF/BC/DE разрушены. Это предел тома,
; а не результат аппаратного определения ёмкости микросхем.
capacity:
    ld hl,(boot+11)
    ld a,l
    or a
    jp nz,bad
    ld a,h
    dec a
    cp 2
    jp nc,bad
    ld c,a
    or a
    jp nz,standardSignature
    ; Том MXOS с 256-байтным сектором узнаётся по OEM, как в DOS.
    ld hl,boot+3
    ld de,oem256
    ld b,8
oemLoop:
    ld a,(de)
    cp (hl)
    jp nz,bad
    inc hl
    inc de
    dec b
    jp nz,oemLoop
    jp totalSize
standardSignature:
    ld a,(boot+510)
    cp 055h
    jp nz,bad
    ld a,(boot+511)
    cp 0AAh
    jp nz,bad
totalSize:
    ld hl,(boot+19)
    ld a,h
    or l
    jp nz,sizeReady
    ld hl,(boot+34)
    ld a,h
    or l
    jp nz,bad
    ld hl,(boot+32)
sizeReady:
    ld a,c
    or a
    jp z,blocksReady
    add hl,hl
    jp c,bad
blocksReady:
    ld a,h
    or l
    jp z,bad
    ld a,h
    cp 020h
    jp c,good
    jp nz,bad
    ld a,l
    or a
    jp nz,bad
good:
    or a
    ret
bad:
    ld a,DOS_ERR_CORRUPT
    scf
    ret
oem256: DB "MXOS256 "
    DEFINE DRIVER_IO_ONLY 1
    INCLUDE "rom-driver.inc"
    UNDEFINE DRIVER_IO_ONLY
boot: DS 512,0
    ENDMODULE
    ELSE
        IF LOCAL_KIND == 1
    DB "AT2MX",0
        ELSE
    DB "AT2MXF",0
        ENDIF
chipDescription: DB "0X512K ",0
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
