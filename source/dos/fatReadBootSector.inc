;----------------------------------------------------------------------------
; MXOS
; Считывание загрузочного сектора и инициализация структуры v_diskInfo.
; Подпрограмма затирает дисковый кэш!
;
; На выходе
;  cf - ошибка
;  регистры bc, de, hl сохраняются
;
; 2022-02-13 Разработано SpaceEngineer
;----------------------------------------------------------------------------

; Структура загрузочного сектора FAT12/16

    STRUCT  FAT_BOOT_SECTOR
BS_JmpBoot      BLOCK   3   ; смещение  00h,   3 байт - Jump instruction to the bootstrap code (x86 instruction): EBh, ??h, 90h (Short jump + NOP) or E9h, ??h, ??h (Near jump).
BS_OEMName      BLOCK   8   ; смещение  03h,   8 байт - "MSWIN 4.1" is recommended but also "MSDOS 5.0" is often used.
BPB_BytsPerSec  BLOCK   2   ; смещение  0Bh,   2 байт - Sector size in unit of byte. Valid values are 512, 1024, 2048, 4096. But most drivers use 512.
BPB_SecPerClus  BLOCK   1   ; смещение  0Dh,   1 байт - Number of sectors per cluster. Valid values are 1, 2, 4, 8, ... 128.
BPB_RsvdSecCnt  BLOCK   2   ; смещение  0Eh,   2 байт - Number of sectors in reserved area. This field hould be 1 on FAT12/16 volume.
BPB_NumFATs     BLOCK   1   ; смещение  10h,   1 байт - Number of FATs. The value of this field should always be 2. On memory cards, it may be 1, but some FAT driver may not recognize such a volume properly.
BPB_RootEntCnt  BLOCK   2   ; смещение  11h,   2 байт - Number of 32-byte directory entries in the root directory. For max compatibility, must be 512 on FAT16 volume.
BPB_TotSec16    BLOCK   2   ; смещение  13h,   2 байт - Total number of sectors of the volume in old 16-bit field. If >= 0x10000, this field = 0, and the true value is set to BPB_TotSec32.
BPB_Media       BLOCK   1   ; смещение  15h,   1 байт - Valid values are F0h, F8h ... FFh. The same value must be put in the lower 8-bits of FAT[0].
BPB_FATSz16     BLOCK   2   ; смещение  16h,   2 байт - Number of sectors occupied by a FAT. The size of the FAT area becomes BPB_FATSz16 * BPB_NumFATs sectors.
BPB_SecPerTrk   BLOCK   2   ; смещение  18h,   2 байт - Number of sectors per track. This field is relevant only for media that have geometry and used for only disk BIOS of IBM PC.
BPB_NumHeads    BLOCK   2   ; смещение  1Ah,   2 байт - Number of heads. This field is relevant only for media that have geometry and used for only disk BIOS of IBM PC.
BPB_HiddSec     BLOCK   4   ; смещение  1Ch,   4 байт - Number of hidden physical sectors preceding the FAT volume. Should always be 0 if the volume starts at the beginning of the storage.
BPB_TotSec32    BLOCK   4   ; смещение  20h,   4 байт - Total number of sectors of the FAT volume in new 32-bit field. 
BS_DrvNum       BLOCK   1   ; смещение  24h,   1 байт - Drive number used by disk BIOS of IBM PC. This field is used in MS-DOS bootstrap, 0x00 for floppy disk and 0x80 for fixed disk.
BS_Reserved     BLOCK   1   ; смещение  25h,   1 байт - Reserved (used by Windows NT). It should be set 0 when create the volume.
BS_BootSig      BLOCK   1   ; смещение  26h,   1 байт - Extended boot signature (0x29). This is a signature byte indicates that the following three fields are present.
BS_VolID        BLOCK   4   ; смещение  27h,   4 байт - Volume serial number used with BS_VolLab to track a volume on the removable storage. It enables to detect a wrong media change by FAT driver.
BS_VolLab       BLOCK  11   ; смещение  2Bh,  11 байт - Volume label, matches volume label recorded in the root directory. When volume label is not present, "NO NAME " should be set.
BS_FilSysType   BLOCK   8   ; смещение  36h,   8 байт - "FAT12   ", "FAT16   " or "FAT     ". Microsoft's FAT driver does not use this field to determine the FAT type.
BS_BootCode     BLOCK 448   ; смещение  3Eh, 448 байт - Bootstrap program. It is platform dependent and filled with zero when not used.
BS_BootSign     BLOCK   2   ; смещение 1FEh,   2 байт - 0xAA55. A boot signature indicating that this is a valid boot sector.
    ENDS

;----------------------------------------------------------------------------

fatReadBootSector:
            push hl
            push de
            push bc
            ld hl,0
            ld (v_diskInfo.rootDirMaxFiles),hl
            ld (v_diskInfo.dataClusters),hl
            ld a,DISK_NO_DRIVER
            ld (v_diskInfo.isValid),a
            ld hl,(j_diskDriver+1)
            ld de,diskDriverDummy
            call cmp_hl_de
            jp z,frbs_NoDriver
            ld a,DISK_INVALID
            ld (v_diskInfo.isValid),a
            ld a,(v_drive)
            ld e,a
            ld d,0
            ld hl,v_blockShifts
            add hl,de
            ld a,(hl)
            ld (v_diskInfo.blockShift),a
            call sectorSizeFromShift
            ld (v_diskInfo.blockSize),hl
            ld b,2
            ld hl,FAT_CACHE_ADDR
            ld de,0
            call j_diskDriver
            jp c,frbs_Io
            ; Старый встроенный ROM получает размер из BPB. Только для него
            ; подготавливаем это поле после первого физического чтения.
            ld a,(v_drive)
            or a
            jp nz,frbs_DriverInfo
            ld hl,(j_diskDriver+1)
            ld de,diskDriver
            call cmp_hl_de
            jp nz,frbs_DriverInfo
            ld hl,(FAT_CACHE_ADDR+19)
            ld (v_diskInfo.totalSectors),hl
frbs_DriverInfo:
            ld b,3
            call j_diskDriver
            jp c,frbs_Io
            ex de,hl
            ld (v_diskInfo.physicalBlocks),hl
            ld h,b
            ld l,c
            ld (v_diskInfo.allocationStep),hl
            ld hl,(FAT_CACHE_ADDR+11)
            ld a,h
            or a
            jp nz,frbs_LargeSector
            ld a,l
            cp 080h
            jp nz,frbs_Bad
            ld a,7
            jp frbs_SectorReady
frbs_LargeSector:
            ld a,l
            or a
            jp nz,frbs_Bad
            ld a,h
            cp 1
            jp z,frbs_256
            cp 2
            jp nz,frbs_Bad
            ld a,9
            jp frbs_SectorReady
frbs_256:
            ld a,8
frbs_SectorReady:
            ld (v_diskInfo.sectorShift),a
            ld (v_diskInfo.sectorSize),hl
            ld hl,v_diskInfo.blockShift
            sub (hl)
            jp c,frbs_Bad
            ld (v_logicalShift),a
            ld b,a
            or a
            jp z,frbs_StepReady
            ld hl,(v_diskInfo.allocationStep)
            ld a,h
            or l
            jp z,frbs_StepReady
            ; При укрупнении сектора отключаем чередование, сохраняя B=4.
            ld hl,1
            ld (v_diskInfo.allocationStep),hl
frbs_StepReady:
            ld hl,(v_diskInfo.physicalBlocks)
frbs_Capacity:
            ld a,b
            or a
            jp z,frbs_CapacityReady
            call div_hl_2
            dec b
            jp frbs_Capacity
frbs_CapacityReady:
            ld (v_logicalCapacity),hl
            call configureSectorCache
            ld hl,FAT_CACHE_ADDR
            ld de,0
            ld b,2
            call logicalSectorIOChecked
            jp c,frbs_Io
            ; 128/256 — явно обозначенные расширения. 512 — стандарт FAT16.
            ld a,(v_diskInfo.sectorShift)
            cp 9
            jp z,frbs_Standard
            ld de,aMxos256
            cp 8
            jp z,frbs_Oem
            ld de,aMxos128
frbs_Oem:
            ld hl,FAT_CACHE_ADDR+3
            ld c,8
            call strcmp
            jp nz,frbs_Bad
            jp frbs_Geometry
frbs_Standard:
            ld hl,(FAT_CACHE_ADDR+510)
            ld de,0AA55h
            call cmp_hl_de
            jp nz,frbs_Bad
frbs_Geometry:
            ; Media допускает F0h и F8h..FFh, остальные значения не FAT.
            ld a,(FAT_CACHE_ADDR+21)
            cp 0F0h
            jp z,frbs_MediaReady
            cp 0F8h
            jp c,frbs_Bad
frbs_MediaReady:
            ld a,(FAT_CACHE_ADDR+13)
            or a
            jp z,frbs_Bad
            ld b,a
            dec a
            and b
            jp nz,frbs_Bad
            ld l,b
            ld h,0
            ld (v_diskInfo.sectorsPerCluster),hl
            ex de,hl
            ld hl,(v_diskInfo.sectorSize)
            call mul_hl_de
            ld a,h
            or l
            jp z,frbs_Bad
            ld de,08000h
            call cmp_hl_de
            jp c,frbs_ClusterReady
            jp nz,frbs_Bad
frbs_ClusterReady:
            ld (v_diskInfo.clusterBytes),hl
            ld a,(FAT_CACHE_ADDR+16)
            dec a
            cp 2
            jp nc,frbs_Bad
            inc a
            ld (v_diskInfo.fatCount),a
            ld hl,(FAT_CACHE_ADDR+19)
            ld a,h
            or l
            jp nz,frbs_TotalReady
            ld hl,(FAT_CACHE_ADDR+34)
            ld a,h
            or l
            jp nz,frbs_Bad
            ld hl,(FAT_CACHE_ADDR+32)
frbs_TotalReady:
            ld (v_diskInfo.totalSectors),hl
            ex de,hl
            ld hl,(v_logicalCapacity)
            call cmp_hl_de
            jp c,frbs_Bad
            ld hl,(FAT_CACHE_ADDR+14)
            ld (v_diskInfo.resSectors),hl
            ld a,h
            or l
            jp z,frbs_Bad
            ld hl,(FAT_CACHE_ADDR+22)
            ld (v_diskInfo.fatSectors),hl
            ld a,h
            or l
            jp z,frbs_Bad
            ld a,(v_diskInfo.sectorShift)
            dec a
frbs_FatCapacity:
            add hl,hl
            jp c,frbs_FatSaturate
            dec a
            jp nz,frbs_FatCapacity
            jp frbs_FatCapacityReady
frbs_FatSaturate:
            ld hl,0FFFFh
frbs_FatCapacityReady:
            ld (v_diskInfo.fatNumCells),hl
            ld hl,(v_diskInfo.fatSectors)
            ld a,(v_diskInfo.fatCount)
            dec a
            jp z,frbs_FatAreaReady
            add hl,hl
            jp c,frbs_Bad
frbs_FatAreaReady:
            ex de,hl
            ld hl,(v_diskInfo.resSectors)
            add hl,de
            jp c,frbs_Bad
            ld (v_diskInfo.dirStartSector),hl
            ld hl,(v_diskInfo.sectorSize)
            call div_hl_2
            call div_hl_2
            call div_hl_2
            call div_hl_2
            call div_hl_2
            ld (v_diskInfo.descrPerSector),hl
            ex de,hl
            ld hl,(FAT_CACHE_ADDR+17)
            ld (v_diskInfo.rootDirMaxFiles),hl
            ld a,h
            or l
            jp z,frbs_Bad
            dec hl
            call div_hl_de
            inc hl
            ld (v_diskInfo.dirSectors),hl
            ex de,hl
            ld hl,(v_diskInfo.dirStartSector)
            add hl,de
            jp c,frbs_Bad
            ld (v_diskInfo.dataStartSector),hl
            ex de,hl
            ld hl,(v_diskInfo.totalSectors)
            call sub_hl_de
            jp c,frbs_Bad
            push hl
            ld hl,(v_diskInfo.sectorsPerCluster)
            ex de,hl
            pop hl
            call div_hl_de
            ld a,h
            or l
            jp z,frbs_Bad
            ld (v_diskInfo.dataClusters),hl
            ld a,(v_diskInfo.sectorShift)
            cp 9
            jp nz,frbs_TypeReady
            ld de,4085
            call cmp_hl_de
            jp c,frbs_Bad      ; FAT12 отдельно, не угадывать по строке FAT16
            ld de,65525
            call cmp_hl_de
            jp nc,frbs_Bad
frbs_TypeReady:
            inc hl
            inc hl
            ex de,hl
            ld hl,(v_diskInfo.fatNumCells)
            call cmp_hl_de
            jp c,frbs_Bad
            ld hl,FAT_CACHE_ADDR+43
            ld de,FAT_CACHE_ADDR+54
            ld bc,v_diskInfo.volumeLabel
            call memcpy_bc_hl
            xor a
            ld (v_diskInfo.volumeLabel+11),a
            ; FAT[0] повторяет BPB_Media, FAT[1] резервирует нижние 14 бит.
            ; Старшие два бита FAT[1] — clean/error flags, допустимы оба
            ; состояния. Проверяем только заголовок, без обхода всего тома.
            ld a,(FAT_CACHE_ADDR+21)
            ld c,a
            ld hl,(v_diskInfo.resSectors)
            ex de,hl
            ld hl,FAT_CACHE_ADDR
            ld b,2
            call logicalSectorIOChecked
            jp c,frbs_Io
            ld a,(hl)
            cp c
            jp nz,frbs_Corrupt
            inc hl
            ld a,(hl)
            cp 0FFh
            jp nz,frbs_Corrupt
            inc hl
            ld a,(hl)
            cp 0FFh
            jp nz,frbs_Corrupt
            inc hl
            ld a,(hl)
            or 0C0h
            cp 0FFh
            jp nz,frbs_Corrupt
            ld a,DISK_VALID
            ld (v_diskInfo.isValid),a
            xor a
            jp frbs_Exit
frbs_Bad:
            ld a,DOS_ERR_UNSUPPORTED
            call dosSetError
            jp frbs_Exit
frbs_NoDriver:
            ld a,DOS_ERR_NO_DRIVER
            call dosSetError
            jp frbs_Exit
frbs_Corrupt:
            call dosCorrupt
            jp frbs_Exit
frbs_Io:
            call dosIoError
frbs_Exit:
            pop bc
            pop de
            pop hl
            ret
aMxos256: DB "MXOS256 "
aMxos128: DB "MXOS128 "
v_logicalCapacity: DW 0
v_logicalShift: DB 0 ; Проверенный log2 числа физических блоков на сектор
