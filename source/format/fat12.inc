FLOPPY_SECTOR_BYTES = 512
FLOPPY_TOTAL_SECTORS = 80*2*9
FLOPPY_FAT_SECTORS = 3
FLOPPY_ROOT_ENTRIES = 112
FLOPPY_ROOT_SECTORS = FLOPPY_ROOT_ENTRIES*32/FLOPPY_SECTOR_BYTES
FLOPPY_ROOT_START = 1+2*FLOPPY_FAT_SECTORS
FLOPPY_MEDIA = 0F9h

;----------------------------------------------------------------------------
; MXOS
;
; Создать FAT12 на дискете PC DD 720 КиБ с готовыми 512-байтными секторами.
;
; вход:
;   Диск выбран и подтверждён. de = число физических секторов, размер
;   блока известен из DOS. v_FormatRAMdisk содержит признак ключа -R.
;
; выход:
;   Записаны две FAT, корень и BPB. При успехе DOS перемонтировала том
;   и открыла корень. При ошибке cf = 1, форматирование прекращается.
;   af, bc, de, hl и секторный буфер рабочие.
;----------------------------------------------------------------------------
FormatFloppy:
    ld hl,FLOPPY_TOTAL_SECTORS
    call bios_cmp_hl_de
    jp nz,errBlockSize
    ld b,0FFh
    call bios_diskDriver
    jp c,errIO
    ld a,b
    cp DISK_TECH_FLOPPY
    jp nz,errBlockSize
    ld a,c
    and DISK_CAP_WRITE
    jp z,errIO
    ld a,(v_FormatRAMdisk)
    or a
    jp nz,errParams
    MXOS_CALL_REGS MXOS_DOS_FLUSH_VOLUME
    jp c,errIO

    call ClearSector
    ld hl,buffer+SECTOR_SIZE
    call ClearSectorPage
    ; В обеих FAT свободные записи нулевые, FAT[0]=FF9h, FAT[1]=FFFh.
    ld hl,0FF00h+FLOPPY_MEDIA
    ld (buffer),hl
    ld a,0FFh
    ld (buffer+2),a
    ld de,1
    ld c,1
    call WriteBuffer
    jp c,FormatFloppyWriteError
    ld de,1+FLOPPY_FAT_SECTORS
    ld c,1
    call WriteBuffer
    jp c,FormatFloppyWriteError
    xor a
    ld (buffer),a
    ld (buffer+1),a
    ld (buffer+2),a
    ld de,2
    ld c,FLOPPY_FAT_SECTORS-1
    call WriteBuffer
    jp c,FormatFloppyWriteError
    ld de,2+FLOPPY_FAT_SECTORS
    ld c,FLOPPY_FAT_SECTORS-1
    call WriteBuffer
    jp c,FormatFloppyWriteError

    ld hl,FLOPPY_ROOT_START
    ld (v_DirStartSector),hl
    ld hl,FLOPPY_ROOT_SECTORS
    ld (v_DirSectors),hl
    ld hl,floppyLabel
    call WriteRoot
    jp c,FormatFloppyWriteError

    ; BPB публикуется после таблиц и каталога.
    ld hl,floppyBoot
    ld de,floppyBootEnd
    ld bc,buffer
    call bios_memcpy_bc_hl
    ld hl,0AA55h
    ld (buffer+FLOPPY_SECTOR_BYTES-2),hl
    ld de,0
    ld c,1
    call WriteBuffer
    jp c,FormatFloppyWriteError
    jp FormatFinish
FormatFloppyWriteError:
    ; При частичной записи прежние записи FAT и корня в кэше устарели.
    MXOS_CALL_REGS MXOS_DOS_MOUNT_VOLUME
    jp errIO

floppyBoot:
    DB 0EBh,03Ch,090h,"MXOS2   "
    DW FLOPPY_SECTOR_BYTES
    DB 2                     ; Секторов на кластер.
    DW 1                     ; Зарезервированный сектор.
    DB 2                     ; Копии FAT.
    DW FLOPPY_ROOT_ENTRIES,FLOPPY_TOTAL_SECTORS
    DB FLOPPY_MEDIA
    DW FLOPPY_FAT_SECTORS,9,2 ; FAT, секторы на дорожку, головки.
    DD 0,0                   ; Скрытые секторы, 32-битный размер тома.
    DB 0,0,029h              ; Номер привода, резерв, сигнатура EBPB.
    DD 0                     ; Серийный номер тома.
floppyLabel:
    DB "MXOS FLOPPY","FAT12   "
    ASSERT $-floppyBoot == 03Eh
    DB 0CDh,018h,0EBh,0FEh   ; x86: возврат в BIOS при попытке загрузки.
floppyBootEnd:
