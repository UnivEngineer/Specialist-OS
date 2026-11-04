; DE=число свободных кластеров, BC/HL сохранены, CF=ошибка.
; Сектор FAT запрашивается один раз, затем записи читаются линейно.
; Горячий путь занятой записи без границы сектора: 85 тактов 8080,
; свободной: ещё 5 тактов INC DE. При совпадении младшего байта конца
; добавляются 27 тактов проверки старшего (112/117). Два чтения FAT
; и одно/два чтения границы из RAM, без портов; смена сектора зависит от I/O.
fatGetDriveFreeSpace:
            call fatDirStart
            ret c
            push hl
            push bc
            ld hl,(v_diskInfo.dataClusters)
            ld b,h
            ld c,l
            ld hl,(v_diskInfo.resSectors)
            ld (v_freeSector),hl
            ld de,0
            call fatFreeSector
            jp c,fatFreeReturn
            inc hl
            inc hl
            inc hl
            inc hl
fatFreeLoop:
            ld a,b
            or c
            jp z,fatFreeDone
            ld a,(hl)
            inc hl
            or (hl)
            inc hl
            jp nz,fatFreeUsed
            inc de
fatFreeUsed:
            dec bc
            ld a,(v_freeEnd)
            cp l
            jp nz,fatFreeLoop
            ld a,(v_freeEnd+1)
            cp h
            jp nz,fatFreeLoop
            ld a,b
            or c
            jp z,fatFreeDone
            ld hl,(v_freeSector)
            inc hl
            ld (v_freeSector),hl
            call fatFreeSector
            jp nc,fatFreeLoop
fatFreeReturn:
            pop bc
            pop hl
            ret
fatFreeDone:
            xor a
            jp fatFreeReturn
fatFreeSector:
            push de
            ld hl,(v_freeSector)
            ex de,hl
            xor a
            call getSectorAddressInCache
            jp c,fatFreeSectorReturn
            push hl
            ex de,hl
            ld hl,(v_diskInfo.sectorSize)
            add hl,de
            ld (v_freeEnd),hl
            pop hl
fatFreeSectorReturn:
            pop de
            ret
v_freeSector: DW 0
v_freeEnd: DW 0
