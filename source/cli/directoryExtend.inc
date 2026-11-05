; Расширение только подкаталога: корень FAT16 остаётся фиксированным.
; Курсор находится на конце цепочки. HL=новая свободная запись, CF/A=ошибка.
; AF/BC/DE/HL разрушены. Нулевой кластер записан до публикации ссылки в FAT.
directoryExtend:
    ld hl,(v_directory)
    ld a,h
    or l
    jp z,directoryRootFull
    call fatFindClusterFirst
    ret c
    ex de,hl
    ld (directoryNewCluster),hl
    ex de,hl
    call fatClusterToSector
    ex de,hl
    ld (directoryNewSector),hl
    ld hl,(v_diskInfo+DISK_INFO.sectorsPerCluster)
    ld (directorySectorsLeft),hl
directoryClearSector:
    ld hl,(directoryNewSector)
    ex de,hl
    ld a,1
    call getSectorAddressInCache
    ret c
    push hl
    ld hl,(v_diskInfo+DISK_INFO.sectorSize)
    ld b,h
    ld c,l
    pop hl
    call cliZero
    ld hl,(directoryNewSector)
    inc hl
    ld (directoryNewSector),hl
    ld hl,(directorySectorsLeft)
    dec hl
    ld (directorySectorsLeft),hl
    ld a,h
    or l
    jp nz,directoryClearSector
    call flushVolume
    ret c
    ld hl,(directoryNewCluster)
    ld de,0FFFFh
    call fatWriteCluster
    jp c,directoryExtendRollback
    call fatFinishWrite
    jp c,directoryExtendRollback
    ld hl,(directoryNewCluster)
    ex de,hl
    ld hl,(v_dirCluster)
    call fatWriteCluster
    jp c,directoryExtendRollback
    call fatFinishWrite
    ret c
    ld hl,(directoryNewCluster)
    ld (v_dirCluster),hl
    ex de,hl
    call fatDirClusterStart
    jp fatDirNext
; Пока ссылка хвоста не опубликована, новый кластер можно освободить.
; После ошибки сохранения самой ссылки сохраняем кластер: ссылка могла записаться.
directoryExtendRollback:
    ld a,(v_dosError)
    push af
    ld hl,(directoryNewCluster)
    ex de,hl
    call fatFreeCluster
    call fatFinishWrite
    pop af
    jp dosSetError
directoryRootFull:
    ld a,DOS_ERR_DIR_FULL
    jp dosSetError
directoryNewCluster: DW 0
directoryNewSector: DW 0
directorySectorsLeft: DW 0
