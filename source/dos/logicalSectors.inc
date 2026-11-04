; Логический сектор состоит из одного или нескольких физических блоков.
; B=1 запись / 2 чтение, DE=LBA тома, HL=буфер полного сектора.
; BC/DE/HL сохранены, AF разрушен, CF=ошибка. Нереентерабельно, DI.
; Секторы меньше блока драйвера отвергаются: скрытой RMW-записи нет.
logicalSectorIO:
            ld a,b
            dec a
            cp 2
            jp nc,blockArgument
            push hl
            ld hl,(v_logicalCapacity)
            call cmp_hl_de
            pop hl
            jp c,blockArgument
            jp z,blockArgument
            ld a,(v_diskInfo.sectorShift)
            push hl
            ld hl,v_diskInfo.blockShift
            sub (hl)
            pop hl
            jp c,blockArgument
            jp z,j_diskDriver
            push hl
            push de
            push bc
            ld c,1
logicalScale:
            push af
            ld a,d
            add a,a
            jp c,logicalOverflow
            ld a,e
            add a,a
            ld e,a
            ld a,d
            rla
            ld d,a
            ld a,c
            add a,a
            ld c,a
            pop af
            dec a
            jp nz,logicalScale
logicalBlocks:
            call j_diskDriver
            jp c,logicalDone
            inc de
            push bc
            push de
            ex de,hl
            ld hl,(v_diskInfo.blockSize)
            add hl,de
            pop de
            pop bc
            dec c
            jp nz,logicalBlocks
            or a
logicalDone:
            pop bc
            pop de
            pop hl
            ret
logicalOverflow:
            pop af
            call blockArgument
            jp logicalDone

; A=log2 размера 7..9. HL=размер в байтах, BC/DE сохранены.
sectorSizeFromShift:
            ld hl,0080h
            sub 7
            ret z
sectorSizeLoop:
            add hl,hl
            dec a
            jp nz,sectorSizeLoop
            ret

; Страйд кэша и число строк меняются вместе с томом: 8/4/2 строки.
configureSectorCache:
            ld hl,(v_diskInfo.sectorSize)
            ld (v_cacheStride),hl
            ex de,hl
            ld hl,FAT_CACHE_SIZE
            call div_hl_de
            ld a,l
            ld (v_cacheCount),a
            jp resetSectorCache
v_cacheStride: DW 256
v_cacheCount: DB FAT_CACHE_CAPACITY

fatCachedWrite:
            call logicalSectorIO
            ret c
            ld a,(v_diskInfo.fatCount)
            cp 2
            jp z,fatMirrorWrite
            or a
            ret
fatMirrorWrite:
            push hl
            push de
            ex de,hl
            push hl
            ld hl,(v_diskInfo.resSectors)
            ex de,hl
            pop hl
            call sub_hl_de
            jp c,fatMirrorDone
            push hl
            ld hl,(v_diskInfo.fatSectors)
            ex de,hl
            pop hl
            call cmp_hl_de
            jp nc,fatMirrorDone
            pop hl                ; исходный сектор
            push hl
            add hl,de
            ex de,hl
            pop hl                ; исходный сектор
            ex (sp),hl            ; HL=буфер, исходный сектор сохранён
            call logicalSectorIO
            pop de
            ret
fatMirrorDone:
            pop de
            pop hl
            or a
            ret

; A=диск 0..7, HL=драйвер, C=log2 блока. HL=DISK_INFO, CF=ошибка.
; BC/DE сохранены; AF разрушается. Перед заменой драйвера нужен flush.
installBlockDriver:
            cp 8
            jp nc,blockArgument
            push af
            ld a,c
            cp 7
            jp c,installBlockBad
            cp 10
            jp nc,installBlockBad
            pop af
            jp installDriverCommon
installBlockBad:
            pop af
blockArgument:
            ld a,DOS_ERR_ARGUMENT
            jp dosSetError

; HL / 2^A, A=6..9. HL=частное, DE=остаток; BC сохранены, AF разрушен.
; Деление 256-байтного сектора сводится к перестановке байтов (42 такта
; без CALL); младшие 6/7 бит FAT и 9-битные секторы обрабатываются отдельно.
splitSectorOffset:
            ld e,l
            ld d,0
            ld l,h
            ld h,0
            cp 8
            ret z
            jp c,splitSmall
            ld a,l
            and 1
            ld d,a
            ld a,l
            or a
            rra
            ld l,a
            ret
splitSmall:
            cp 7
            jp z,splitSeven
            add hl,hl
            add hl,hl
            ld a,e
            rlca
            rlca
            and 3
            add a,l
            ld l,a
            ld a,e
            and 03Fh
            ld e,a
            ret
splitSeven:
            add hl,hl
            ld a,e
            rlca
            and 1
            add a,l
            ld l,a
            ld a,e
            and 07Fh
            ld e,a
            ret
