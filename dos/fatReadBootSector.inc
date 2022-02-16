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
BPB_BytsPerSec  BLOCK   2   ; смещение  0Bh,   2 байт - Sector size in unit of byte. Valid values are 512, 1024, 2048, 4096. But must drivers use 512.
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
            ; Сохраняем регистры
            push    hl
            push    de
            push    bc

            ; Помещаем значение DISK_NO_DRIVER в v_diskInfo.isValid
            ld      a, DISK_NO_DRIVER
            ld      (v_diskInfo.isValid), a

            ; Проверяем, установлен ли драйвер: сравниваем
            ; с адресом пустого драйвера diskDriverDummy
            ld      hl, (j_diskDriver + 1)  ; hl = адрес драйвера
            ld      a, diskDriverDummy >> 8
            cp      h
            jp nz,  frbs_DriverOk
            ld      a, diskDriverDummy & 0FFh
            cp      h
            jp z,   frbs_Exit

frbs_DriverOk:
            ; Драйвер установлен; загружаем boot сектор в дисковый кэш, затирая его
            ld      hl, FAT_CACHE_ADDR  ; адрес буфера
            ld      de, 0               ; номер сектора - 0
            ld      b, 2                ; режим драйвера - чтение
            call    j_diskDriver        ; читаем сектор в память

            ; Помещаем значение DISK_INVALID в v_diskInfo.isValid
            ld      a, DISK_INVALID
            ld      (v_diskInfo.isValid), a

            ; Проверяем, корректный ли это FAT12/16 диск:
            ; поле BS_FilSysType должно содержить текст "FAT"
            ld      hl, FAT_CACHE_ADDR + FAT_BOOT_SECTOR.BS_FilSysType
            ld      de, aFat
            ld      c, 3
            call    strcmp
            jp nz,  frbs_Exit   ; это не FAT диск, или не отформатирован - выходим

            ; Помещаем значение DISK_VALID в v_diskInfo.isValid
            ld      a, DISK_VALID
            ld      (v_diskInfo.isValid), a

            ; Копируем и рассчитываем необходимые значения
            ld      hl, (FAT_CACHE_ADDR + FAT_BOOT_SECTOR.BPB_BytsPerSec)
            ld      (v_diskInfo.sectorSize), hl         ; размер сектора

            ld      a, (FAT_CACHE_ADDR + FAT_BOOT_SECTOR.BPB_SecPerClus)
            ld      (v_diskInfo.sectorsPerCluster), a
            xor     a
            ld      (v_diskInfo.sectorsPerCluster+1), a ; секторов на кластер (16-битным словом)

            ld      hl, (FAT_CACHE_ADDR + FAT_BOOT_SECTOR.BPB_TotSec16)
            ld      (v_diskInfo.totalSectors), hl       ; сколько всего секторов на диске

            ld      hl, (FAT_CACHE_ADDR + FAT_BOOT_SECTOR.BPB_RsvdSecCnt)
            ld      (v_diskInfo.resSectors), hl         ; сколько секторов занимает зарезервированная область

            ex      hl, de

            ld      hl, (FAT_CACHE_ADDR + FAT_BOOT_SECTOR.BPB_FATSz16)
            ld      (v_diskInfo.fatSectors), hl         ; сколько секторов занимает таблица fat

            add     hl, de
            ld      (v_diskInfo.dirStartSector), hl     ; первый сектор корневого каталога
            push    hl

            ld      hl, (FAT_CACHE_ADDR + FAT_BOOT_SECTOR.BPB_RootEntCnt)
            ld      (v_diskInfo.rootDirMaxFiles), hl    ; максимум файлов в корневом каталоге

            ld      hl, (v_diskInfo.sectorSize)
            ld      de, DIR_DESCR_SIZE  ; de = 32
            call    div_hl_de;
            ld      (v_diskInfo.descrPerSector), hl     ; сколько дескрипторов файлов вмещается в один сектор

            ex      hl, de
            ld      hl, (v_diskInfo.rootDirMaxFiles)
            call    div_hl_de;
            ld      (v_diskInfo.dirSectors), hl         ; сколько секторов занимает корневой каталог

            pop     de
            add     hl, de
            ld      (v_diskInfo.dataStartSector), hl    ; первый сектор области данных

            ld      hl, (v_diskInfo.sectorSize)
            call    div_hl_2
            ex      hl, de
            ld      hl, (v_diskInfo.fatSectors)
            call    mul_hl_de;
            ld      (v_diskInfo.fatNumCells), hl        ; количество ячеек в таблице fat (= fatSectors * sectorSize / 2)

            ; Копируем метку тома
            ld      hl, FAT_CACHE_ADDR + FAT_BOOT_SECTOR.BS_VolLab          ; откуда - начало
            ld      de, FAT_CACHE_ADDR + FAT_BOOT_SECTOR.BS_FilSysType      ; откуда - конец
            ld      bc, v_diskInfo.volumeLabel                              ; куда
            call    memcpy_bc_hl
            ld      (hl), 0                                                 ; в конце байт-терминатор строки

frbs_Exit:  ; Восстанавливаем регистры и выходим
            pop     bc
            pop     de
            pop     hl
            ret

