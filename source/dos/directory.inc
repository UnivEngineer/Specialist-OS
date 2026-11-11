; Общий курсор корня. Начало не меняет BC/DE/HL, AF разрушается.
fatDirStart:
            call fatCheckMounted
            ret c
            push hl
            push de
            xor a
            ld (v_dirRemaining),a
            ld hl,(v_directory)
            ld a,h
            or l
            jp z,fatDirRootStart
            ex de,hl
            call fatCheckCluster
            jp c,fatDirStartReturn
            ex de,hl
            ld (v_dirCluster),hl
            ex de,hl
            ld hl,(v_diskInfo.dataClusters)
            dec hl
            ld (v_dirBudget),hl
            call fatDirClusterStart
            jp fatDirStartReturn
fatDirRootStart:
            ld hl,(v_diskInfo.rootDirMaxFiles)
            ld (v_dirRootRemaining),hl
            ld hl,(v_diskInfo.dirStartSector)
            dec hl
            ld (v_cachedSector),hl
            or a
fatDirStartReturn:
            pop de
            pop hl
            ret
fatCheckMounted:
            ld a,(v_diskInfo.isValid)
            cp DISK_VALID
            ret z
            add a,DOS_ERR_NO_DRIVER
            jp dosSetError
; Следующая физическая запись: HL=дескриптор, DE/BC сохраняются.
; CF=1: A=0 конец корня, A!=0 ошибка. Пустая запись 00h тоже возвращается.
fatDirNext:
            push de
            push bc
            ld hl,(v_directory)
            ld a,h
            or l
            jp nz,fatDirEntryReady
            ld hl,(v_dirRootRemaining)
            ld a,h
            or l
            jp z,fatDirEnd
            dec hl
            ld (v_dirRootRemaining),hl
fatDirEntryReady:
            ; Во время прохода каталога кэш не используется другими циклами.
            ; Сектор получаем один раз, остальные записи идут по указателю.
            ld a,(v_dirRemaining)
            or a
            jp z,fatDirLoadSector
            dec a
            ld (v_dirRemaining),a
            ld de,FILE_DESCR_SIZE
            ld hl,(v_cachedDescrOffset)
            add hl,de
            ld (v_cachedDescrOffset),hl
            ld hl,(v_cachedDescrPtr)
            add hl,de
            ld (v_cachedDescrPtr),hl
            or a
            jp fatDirReturn
fatDirLoadSector:
            ld hl,(v_directory)
            ld a,h
            or l
            jp z,fatDirLoadRoot
            ld hl,(v_dirSectorsLeft)
            ld a,h
            or l
            jp nz,fatDirUseSector
            ld hl,(v_dirCluster)
            ex de,hl
            call fatReadCluster
            jp c,fatDirReturn
            ld a,d
            cp 0FFh
            jp nz,fatDirFollow
            ld a,e
            cp 0F8h
            jp nc,fatDirEnd
fatDirFollow:
            call fatCheckCluster
            jp c,fatDirReturn
            ld hl,(v_dirBudget)
            ld a,h
            or l
            jp z,fatDirBadChain
            dec hl
            ld (v_dirBudget),hl
            ex de,hl
            ld (v_dirCluster),hl
            ex de,hl
            call fatDirClusterStart
            ld hl,(v_dirSectorsLeft)
fatDirUseSector:
            dec hl
            ld (v_dirSectorsLeft),hl
fatDirLoadRoot:
            ld hl,(v_cachedSector)
            inc hl
            ld (v_cachedSector),hl
            ex de,hl
            xor a
            call getSectorAddressInCache
            jp c,fatDirReturn
            ld (v_cachedDescrPtr),hl
            ld hl,0
            ld (v_cachedDescrOffset),hl
            ld a,(v_diskInfo.descrPerSector)
            dec a
            ld (v_dirRemaining),a
            ld hl,(v_cachedDescrPtr)
            or a
fatDirReturn:
            pop bc
            pop de
            ret
fatDirEnd:
            xor a
            scf
            jp fatDirReturn
fatDirBadChain:
            call dosCorrupt
            jp fatDirReturn
; DE=проверенный кластер; BC сохранён, HL/AF разрушены.
fatDirClusterStart:
            push de
            call fatClusterToSector
            ex de,hl
            dec hl
            ld (v_cachedSector),hl
            ld hl,(v_diskInfo.sectorsPerCluster)
            ld (v_dirSectorsLeft),hl
            pop de
            or a
            ret
; Следующий обычный файл. 00h завершает каталог, E5h пропускается.
; Каталоги, метки и LFN пока не выдаются за файлы. Регистры как fatDirNext.
fatDirFile:
            ld a,018h
            jp fatDirFiltered
fatDirItem:
            ld a,008h
fatDirFiltered:
            ld (fatDirAttrMask+1),a
fatDirFilteredLoop:
            call fatDirNext
            ret c
            ld a,(hl)
            or a
            jp z,fatDirEndFile
            cp 0E5h
            jp z,fatDirFilteredLoop
            push hl
            push de
            ld de,FILE_DESCRIPTOR.attrib
            add hl,de
            ld a,(hl)
fatDirAttrMask:
            and 018h
            pop de
            pop hl
            jp nz,fatDirFilteredLoop
            or a
            ret
fatDirEndFile:
            scf
            ret

; HL=свободная запись. При замене 00h переносим конец каталога вперёд:
; байты после прежнего конца не обязаны быть нулевыми. E5h не сдвигает конец.
; BC/DE сохранены, HL=актуальный указатель выбранной записи; AF разрушен.
; CF=ошибка. Курсор обхода после вызова больше не используется.
fatDirPrepareEntry:
            ld a,(hl)
            or a
            ret nz
            push bc
            push de
            ld hl,(v_cachedSector)
            push hl
            ld hl,(v_cachedDescrOffset)
            push hl
            call fatDirNext
            jp nc,fatDirMoveEnd
            ; A=0 означает последнюю запись корня: продолжения не нужно.
            or a
            jp z,fatDirRestoreEntry
            scf
            jp fatDirRestoreEntry
fatDirMoveEnd:
            call markDirSectorAsModified
            jp c,fatDirRestoreEntry
            ld (hl),0
fatDirRestoreEntry:
            pop hl
            ld (v_cachedDescrOffset),hl
            pop hl
            ld (v_cachedSector),hl
            pop de
            pop bc
            ret c
            ; Загрузка соседнего сектора могла вытеснить выбранную запись.
            jp markDirSectorAsModified

v_dirRemaining: DB 0
v_dirRootRemaining: DW 0
v_cachedDescrOffset: DW 0
v_dirCluster: DW 0
v_dirSectorsLeft: DW 0
v_dirBudget: DW 0

; HL=запись каталога. E5h в первой букве имени хранится как 05h.
; HL/BC/DE сохранены, AF разрушается.
fatEncodeName:
            ld a,(hl)
            cp 0E5h
            ret nz
            ld (hl),5
            ret
