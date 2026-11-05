; Каталоги FAT16. A=DIR_OP_*, параметры по mxos.inc.
; AF/BC/DE/HL разрушены; CF/A — числовой статус DOS. SET принимает только
; ранее полученный кластер каталога, не произвольный номер из приложения.
; CHDIR: буфер MAX_PATH принадлежит вызывающему, содержит 0, не меняется.
; Префикс диска необязателен; / и \ равнозначны, корень — кластер 0.
directoryService:
            cp DIR_OP_MKDIR
            jp z,directoryCreate
            cp DIR_OP_RMDIR
            jp z,directoryRemove
            cp DIR_OP_RESOLVED
            jp z,directoryGet
            cp DIR_OP_COMMAND
            jp z,directoryCommand
            cp DIR_OP_CHDIR
            jp z,directoryPath
            call selectDirectory
            push af
            xor a
            ld (v_dosError),a
            pop af
            push af
            push de
            call fatCheckMounted
            jp c,directoryMountError
            push hl
            ld hl,(v_volumeTable)
            ld de,fatVolumeDriver+VOLUME_DRIVER_HEADER
            call cmp_hl_de
            pop hl
            pop de
            jp nz,directoryUnsupported
            pop af
            cp DIR_OP_GETCWD
            jp z,directoryGetCwd
            cp DIR_OP_LIST
            jp z,fatDirectoryList
            or a
            jp z,directoryGet
            dec a
            jp z,directorySet
            jp blockArgument
directoryMountError:
            pop de
            pop bc
            ret
directoryUnsupported:
            pop af
            jp dosUnsupported
directoryGet:
            ld hl,(v_directory)
            ex de,hl
            jp directorySuccess
directorySet:
            ld a,d
            or e
            call nz,fatCheckCluster
            ret c
            ex de,hl
            ld (v_directory),hl
            jp directoryCommit

; Выбор контекста после смены диска, включая повторный выбор того же диска.
; BC/DE/HL и A сохранены; CF=0.
selectDirectory:
            push af
            push hl
            push de
            call directorySlot
            ld e,(hl)
            inc hl
            ld d,(hl)
            ex de,hl
            ld (v_directory),hl
            pop de
            pop hl
            pop af
            or a
            ret
directorySlot:
            ld a,(v_drive)
            add a,a
            ld e,a
            ld d,0
            ld hl,v_directories
            add hl,de
            ret
directoryCommit:
            call directorySlot
            ex de,hl
            ld hl,(v_directory)
            ld a,l
            ld (de),a
            inc de
            ld a,h
            ld (de),a
directorySuccess:
            xor a
            ld (v_dosError),a
            ret

directoryPath:
            ld bc,MAX_PATH
            call streamBufferRange
            ret c
directoryPathUnchecked:
            ; Проверка терминатора до дискового обхода: нет молчаливого усечения.
            push hl
            ld b,MAX_PATH
directoryLength:
            ld a,(hl)
            or a
            jp z,directoryLengthOk
            inc hl
            dec b
            jp nz,directoryLength
            pop hl
            jp blockArgument
directoryLengthOk:
            ld (v_pathEnd),hl
            pop hl
            ld a,(v_drive)
            push af
            call directoryDrive
            jp c,directoryDriveRollback
            call fatCheckMounted
            jp c,directoryDriveRollback
            ; Один префикс A: означает сохранённый текущий каталог этого диска.
            ld a,(hl)
            or a
            jp z,directoryPathReady
            call directoryParse
            jp c,directoryDriveRollback
directoryPathReady:
            pop bc
            jp directoryCommit
directoryDriveRollback:
            ld a,(v_dosError)
            ld b,a
            pop af
            push bc
            call fileSelectDrive
            pop bc
            ld a,b
            jp dosSetError

; HL=путь, v_pathEnd=исключительная граница. Без изменения сохранённого CWD.
; Выход HL=граница, CF=ошибка; остальные регистры разрушены.
directoryParse:
            ld a,(hl)
            or a
            jp z,blockArgument
            push hl
            ld hl,(v_directory)
            ld (v_pathOld),hl
            pop hl
            ld a,(hl)
            call directorySeparator
            jp nz,directoryComponent
            push hl
            ld hl,0
            ld (v_directory),hl
            pop hl
directorySkip:
            call directoryAtEnd
            ret z
            ld a,(hl)
            call directorySeparator
            jp nz,directoryComponent
            inc hl
            jp directorySkip
directoryComponent:
            call directoryAtEnd
            ret z
            ld a,(hl)
            or a
            ret z
            push hl
            ld de,v_pathName
            ld b,11
            call memset_de_20_b
            pop hl
            ld de,v_pathName
            ld bc,0800h
            ld a,(hl)
            cp '.'
            jp nz,directoryName
            ; Только отдельные . и .., не длинные имена с начальной точкой.
            ld (de),a
            inc de
            inc hl
            ld a,(hl)
            cp '.'
            jp nz,directoryDotEnd
            ld (de),a
            inc hl
            ld a,(hl)
directoryDotEnd:
            or a
            jp z,directoryLookup
            call directorySeparator
            jp z,directoryLookup
            jp directoryBadPath
directoryName:
            ld a,(hl)
            or a
            jp z,directoryNameEnd
            call directorySeparator
            jp z,directoryNameEnd
            cp '.'
            jp z,directoryExtension
            cp 021h
            jp c,directoryBadPath
            cp ':'
            jp z,directoryBadPath
            cp '*'
            jp z,directoryBadPath
            cp '?'
            jp z,directoryBadPath
            cp '"'
            jp z,directoryBadPath
            cp '<'
            jp z,directoryBadPath
            cp '>'
            jp z,directoryBadPath
            cp '|'
            jp z,directoryBadPath
            inc b
            dec b
            jp z,directoryBadPath
            ld (de),a
            inc de
            inc hl
            dec b
            jp directoryName
directoryExtension:
            ld a,c
            or a
            jp nz,directoryBadPath
            ld a,b
            cp 8
            jp z,directoryBadPath
            ld de,v_pathName+8
            ld bc,0301h
            inc hl
            jp directoryName
directoryNameEnd:
            ld a,c
            or a
            jp z,directoryLookup
            ld a,b
            cp 3
            jp z,directoryBadPath
directoryLookup:
            push hl
            ld a,(v_pathName)
            cp '.'
            jp nz,directoryFind
            ld a,(v_pathName+1)
            cp ' '
            jp z,directoryNext
            ld hl,(v_directory)
            ld a,h
            or l
            jp z,directoryNext
directoryFind:
            ld hl,v_pathName
            call fileFindAny
            jp c,directoryFindFailed
            ld a,(v_tmpFileDescr+FILE_DESCRIPTOR.attrib)
            and 010h
            jp z,directoryNotDir
            ld hl,(v_tmpFileDescr+FILE_DESCRIPTOR.firstCluster)
            ld a,h
            or l
            jp nz,directoryCheckChild
            ld a,(v_pathName)
            cp '.'
            jp nz,directoryBadChild
            jp directoryChildOk
directoryCheckChild:
            ex de,hl
            call fatCheckCluster
            jp c,directoryFindFailed
            ex de,hl
directoryChildOk:
            ld (v_directory),hl
directoryNext:
            pop hl
            jp directorySkip
directoryNotDir:
            ld a,DOS_ERR_ACCESS
            call dosSetError
            jp directoryFindFailed
directoryBadChild:
            call dosCorrupt
directoryFindFailed:
            pop hl
            jp directoryRestore
directoryBadPath:
            call blockArgument
directoryRestore:
            ld hl,(v_pathOld)
            ld (v_directory),hl
            ret
directorySeparator:
            cp '/'
            ret z
            cp 05Ch
            ret

; Проверка конца сохраняет все регистры; Z/CF=0 означают конец.
directoryAtEnd:
            push de
            push hl
            ld hl,(v_pathEnd)
            ex de,hl
            pop hl
            call cmp_hl_de
            pop de
            ret nz
            xor a
            ret
; Необязательный префикс A:, регистр буквы несущественен.
; BC/DE сохранены, HL продвигается после двоеточия.
directoryDrive:
            call selectDirectory
            inc hl
            ld a,(hl)
            dec hl
            cp ':'
            jp nz,directoryDriveReady
            ld a,(hl)
            and 0DFh
            sub 'A'
            call fileChangeDrive
            ret c
            inc hl
            inc hl
directoryDriveReady:
            or a
            ret
v_pathEnd: DW 0
v_directory: DW 0
v_directories: DS 16,0
v_pathOld: DW 0
v_pathName: DS 11,0
