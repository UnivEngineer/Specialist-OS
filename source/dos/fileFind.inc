; HL=компактное имя. BC/DE/HL сохранены, CF=ошибка.
; Дескриптор копируется в v_tmpFileDescr; указатель кэша не долговечен.
fileFind:
            ld a,018h
            jp fileFindStart
fileFindAny:
            ld a,008h
fileFindStart:
            ld (v_findMask),a
            ld (v_foundDescrPtr),hl
            push hl
            push de
            push bc
            ld a,(hl)
            call upperCase
            ld (fatDirFindFirst+1),a
            call fatDirStart
            jp c,fileFind_end
            ld a,(v_findMask)
            call fatDirFind
            jp c,fileFind_end
            ld hl,(v_cachedDescrPtr)
            ex de,hl
            ld hl,v_tmpFileDescr
            ld (v_foundDescrPtr),hl
            call copyDescriptor
            ; FstClusHI зарезервирован в FAT16, ненулевое значение — ошибка.
            ld hl,(v_tmpFileDescr+FILE_DESCRIPTOR.firstClusterHi)
            ld a,h
            or l
            jp z,fileFind_highClusterOk
            call dosCorrupt
            jp fileFind_returnError
fileFind_highClusterOk:
            ld a,(v_tmpFileDescr)
            cp 5
            jp nz,fileFind_found
            ld a,0E5h
            ld (v_tmpFileDescr),a
fileFind_found:
            pop bc
            pop de
            pop hl
            or a
            ret
fileFind_end:
            or a
            jp nz,fileFind_returnError
            ld a,DOS_ERR_NOT_FOUND
            call dosSetError
            ld a,3
fileFind_returnError:
            pop bc
            pop de
            pop hl
            scf
            ret
v_findMask: DB 018h
