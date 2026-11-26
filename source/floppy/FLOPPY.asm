; ВГ93 «Специалиста-МХ». Выбор, позиционирование и аппаратный запрос
; FFF0h портированы из floppy.a80 (Fifan, 2016). Физическая геометрия
; PC DD: 80 цилиндров, 2 стороны, 9 секторов MFM по 512 байт.
    INCLUDE "../common/system.inc"
IMAGE_FLAGS = IMAGE_FLAG_DRIVER
    IFNDEF DRIVER_ORIGIN
DRIVER_ORIGIN = 0
    ENDIF
    ORG DRIVER_ORIGIN
    RELOCATE_START
driverBody:

;----------------------------------------------------------------------------
; MXOS
;
; Установить физический дисковод 0 на D:. Дискета при установке не требуется.
; вход: страница DOS, загруженный FILE_EXEC установщик.
; выход: cf=0 — служба зарегистрирована; cf=1, a=ошибка DOS. bc/de/hl рабочие.
;----------------------------------------------------------------------------
    ex de,hl
    ld c,FLOPPY_DRIVE
    call driverLetter
    ret c
    push af
    call argNext
    pop bc
    jp nc,driverLetterBad
    ld a,b
    ld (registration+DRIVER_INSTALL.drive),a
    ld hl,registration
    MXOS_CALL_REGS MXOS_DOS_DRIVER_INSTALL
    ret
    INCLUDE "../common/argNext.inc"
    INCLUDE "../common/driver-letter.inc"
registration:
    DB DRIVER_FLOPPY_RELOC,FLOPPY_DRIVE,DRIVER_KIND_FLOPPY,9
    DW 80*2*9,0
    DB DISK_CAP_READ+DISK_CAP_WRITE+DISK_CAP_VERIFY,DISK_TECH_FLOPPY
    DW serviceImage,serviceEnd-serviceImage,description
description: DB "MX FDD",0,"VG93 720K DD",0

serviceImage:
    INCBIN "../../bin/FLOPPYSVC.bin"
serviceEnd:
    RELOCATE_END
IMAGE_BODY_SIZE  = $-driverBody
IMAGE_MEM_BEGIN  = 0
IMAGE_MEM_END    = IMAGE_BODY_SIZE
IMAGE_BSS_SIZE   = 0
IMAGE_RELOC_KIND = 0
IMAGE_ALIGN_LOG2 = 0
    RELOCATE_TABLE driverBody
    END
