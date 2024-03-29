;----------------------------------------------------------------------------
; MXOS
; ���������� ������������ ������� � ������������� ��������� v_diskInfo.
; ������������ �������� �������� ���!
;
; �� ������
;  cf - ������
;  �������� bc, de, hl �����������
;
; 2022-02-13 ����������� SpaceEngineer
;----------------------------------------------------------------------------

; ��������� ������������ ������� FAT12/16

    STRUCT  FAT_BOOT_SECTOR
BS_JmpBoot      BLOCK   3   ; ��������  00h,   3 ���� - Jump instruction to the bootstrap code (x86 instruction): EBh, ??h, 90h (Short jump + NOP) or E9h, ??h, ??h (Near jump).
BS_OEMName      BLOCK   8   ; ��������  03h,   8 ���� - "MSWIN 4.1" is recommended but also "MSDOS 5.0" is often used.
BPB_BytsPerSec  BLOCK   2   ; ��������  0Bh,   2 ���� - Sector size in unit of byte. Valid values are 512, 1024, 2048, 4096. But must drivers use 512.
BPB_SecPerClus  BLOCK   1   ; ��������  0Dh,   1 ���� - Number of sectors per cluster. Valid values are 1, 2, 4, 8, ... 128.
BPB_RsvdSecCnt  BLOCK   2   ; ��������  0Eh,   2 ���� - Number of sectors in reserved area. This field hould be 1 on FAT12/16 volume.
BPB_NumFATs     BLOCK   1   ; ��������  10h,   1 ���� - Number of FATs. The value of this field should always be 2. On memory cards, it may be 1, but some FAT driver may not recognize such a volume properly.
BPB_RootEntCnt  BLOCK   2   ; ��������  11h,   2 ���� - Number of 32-byte directory entries in the root directory. For max compatibility, must be 512 on FAT16 volume.
BPB_TotSec16    BLOCK   2   ; ��������  13h,   2 ���� - Total number of sectors of the volume in old 16-bit field. If >= 0x10000, this field = 0, and the true value is set to BPB_TotSec32.
BPB_Media       BLOCK   1   ; ��������  15h,   1 ���� - Valid values are F0h, F8h ... FFh. The same value must be put in the lower 8-bits of FAT[0].
BPB_FATSz16     BLOCK   2   ; ��������  16h,   2 ���� - Number of sectors occupied by a FAT. The size of the FAT area becomes BPB_FATSz16 * BPB_NumFATs sectors.
BPB_SecPerTrk   BLOCK   2   ; ��������  18h,   2 ���� - Number of sectors per track. This field is relevant only for media that have geometry and used for only disk BIOS of IBM PC.
BPB_NumHeads    BLOCK   2   ; ��������  1Ah,   2 ���� - Number of heads. This field is relevant only for media that have geometry and used for only disk BIOS of IBM PC.
BPB_HiddSec     BLOCK   4   ; ��������  1Ch,   4 ���� - Number of hidden physical sectors preceding the FAT volume. Should always be 0 if the volume starts at the beginning of the storage.
BPB_TotSec32    BLOCK   4   ; ��������  20h,   4 ���� - Total number of sectors of the FAT volume in new 32-bit field. 
BS_DrvNum       BLOCK   1   ; ��������  24h,   1 ���� - Drive number used by disk BIOS of IBM PC. This field is used in MS-DOS bootstrap, 0x00 for floppy disk and 0x80 for fixed disk.
BS_Reserved     BLOCK   1   ; ��������  25h,   1 ���� - Reserved (used by Windows NT). It should be set 0 when create the volume.
BS_BootSig      BLOCK   1   ; ��������  26h,   1 ���� - Extended boot signature (0x29). This is a signature byte indicates that the following three fields are present.
BS_VolID        BLOCK   4   ; ��������  27h,   4 ���� - Volume serial number used with BS_VolLab to track a volume on the removable storage. It enables to detect a wrong media change by FAT driver.
BS_VolLab       BLOCK  11   ; ��������  2Bh,  11 ���� - Volume label, matches volume label recorded in the root directory. When volume label is not present, "NO NAME " should be set.
BS_FilSysType   BLOCK   8   ; ��������  36h,   8 ���� - "FAT12   ", "FAT16   " or "FAT     ". Microsoft's FAT driver does not use this field to determine the FAT type.
BS_BootCode     BLOCK 448   ; ��������  3Eh, 448 ���� - Bootstrap program. It is platform dependent and filled with zero when not used.
BS_BootSign     BLOCK   2   ; �������� 1FEh,   2 ���� - 0xAA55. A boot signature indicating that this is a valid boot sector.
    ENDS

;----------------------------------------------------------------------------

fatReadBootSector:
            ; ��������� ��������
            push    hl
            push    de
            push    bc

            ; �������� �������� DISK_NO_DRIVER � v_diskInfo.isValid
            ld      a, DISK_NO_DRIVER
            ld      (v_diskInfo.isValid), a

            ; ���������, ���������� �� �������: ����������
            ; � ������� ������� �������� diskDriverDummy
            ld      hl, (j_diskDriver + 1)  ; hl = ����� ��������
            ld      a, diskDriverDummy >> 8
            cp      h
            jp nz,  frbs_DriverOk
            ld      a, diskDriverDummy & 0FFh
            cp      h
            jp z,   frbs_Exit

frbs_DriverOk:
            ; ������� ����������; ��������� boot ������ � �������� ���, ������� ���
            ld      hl, FAT_CACHE_ADDR  ; ����� ������
            ld      de, 0               ; ����� ������� - 0
            ld      b, 2                ; ����� �������� - ������
            call    j_diskDriver        ; ������ ������ � ������

            ; �������� �������� DISK_INVALID � v_diskInfo.isValid
            ld      a, DISK_INVALID
            ld      (v_diskInfo.isValid), a

            ; ���������, ���������� �� ��� FAT12/16 ����:
            ; ���� BS_FilSysType ������ ��������� ����� "FAT"
            ld      hl, FAT_CACHE_ADDR + FAT_BOOT_SECTOR.BS_FilSysType
            ld      de, aFat
            ld      c, 3
            call    strcmp
            jp nz,  frbs_Exit   ; ��� �� FAT ����, ��� �� �������������� - �������

            ; �������� �������� DISK_VALID � v_diskInfo.isValid
            ld      a, DISK_VALID
            ld      (v_diskInfo.isValid), a

            ; �������� � ������������ ����������� ��������
            ld      hl, (FAT_CACHE_ADDR + FAT_BOOT_SECTOR.BPB_BytsPerSec)
            ld      (v_diskInfo.sectorSize), hl         ; ������ �������

            ld      a, (FAT_CACHE_ADDR + FAT_BOOT_SECTOR.BPB_SecPerClus)
            ld      (v_diskInfo.sectorsPerCluster), a
            xor     a
            ld      (v_diskInfo.sectorsPerCluster+1), a ; �������� �� ������� (16-������ ������)

            ld      hl, (FAT_CACHE_ADDR + FAT_BOOT_SECTOR.BPB_TotSec16)
            ld      (v_diskInfo.totalSectors), hl       ; ������� ����� �������� �� �����

            ld      hl, (FAT_CACHE_ADDR + FAT_BOOT_SECTOR.BPB_RsvdSecCnt)
            ld      (v_diskInfo.resSectors), hl         ; ������� �������� �������� ����������������� �������

            ex      hl, de

            ld      hl, (FAT_CACHE_ADDR + FAT_BOOT_SECTOR.BPB_FATSz16)
            ld      (v_diskInfo.fatSectors), hl         ; ������� �������� �������� ������� fat

            add     hl, de
            ld      (v_diskInfo.dirStartSector), hl     ; ������ ������ ��������� ��������
            push    hl

            ld      hl, (FAT_CACHE_ADDR + FAT_BOOT_SECTOR.BPB_RootEntCnt)
            ld      (v_diskInfo.rootDirMaxFiles), hl    ; �������� ������ � �������� ��������

            ld      hl, (v_diskInfo.sectorSize)
            ld      de, FILE_DESCR_SIZE ; de = 32
            call    div_hl_de;
            ld      (v_diskInfo.descrPerSector), hl     ; ������� ������������ ������ ��������� � ���� ������

            ex      hl, de
            ld      hl, (v_diskInfo.rootDirMaxFiles)
            call    div_hl_de;
            ld      (v_diskInfo.dirSectors), hl         ; ������� �������� �������� �������� �������

            pop     de
            add     hl, de
            ld      (v_diskInfo.dataStartSector), hl    ; ������ ������ ������� ������

            ld      hl, (v_diskInfo.sectorSize)
            call    div_hl_2
            ex      hl, de
            ld      hl, (v_diskInfo.fatSectors)
            call    mul_hl_de;
            ld      (v_diskInfo.fatNumCells), hl        ; ���������� ����� � ������� fat (= fatSectors * sectorSize / 2)

            ; �������� ����� ����
            ld      hl, FAT_CACHE_ADDR + FAT_BOOT_SECTOR.BS_VolLab          ; ������ - ������
            ld      de, FAT_CACHE_ADDR + FAT_BOOT_SECTOR.BS_FilSysType      ; ������ - �����
            ld      bc, v_diskInfo.volumeLabel                              ; ����
            call    memcpy_bc_hl
            ld      (hl), 0                                                 ; � ����� ����-���������� ������

frbs_Exit:  ; ��������������� �������� � �������
            pop     bc
            pop     de
            pop     hl
            ret

