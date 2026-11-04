; HL=компактное имя. BC/DE/HL сохранены, CF=ошибка.
; Дескриптор копируется в v_tmpFileDescr; указатель кэша не долговечен.
fileFind:
            ld (v_foundDescrPtr),hl
            push hl
            push de
            push bc
            call fatDirStart
            jp c,fileFind_end
fileFind_loop:
            call fatDirFile
            jp c,fileFind_end
            ex de,hl
            ld hl,(v_foundDescrPtr)
            push hl
            ld c,FILE_NAME_LENGTH+3
            ld a,(de)
            cp 5
            jp nz,fileFind_compare
            ld a,(hl)
            call upperCase
            cp 0E5h
            jp nz,fileFind_notEqual
            inc hl
            inc de
            dec c
fileFind_compare:
            call stricmp
fileFind_notEqual:
            pop hl
            jp nz,fileFind_loop
            ld hl,(v_cachedDescrPtr)
            ex de,hl
            ld hl,v_tmpFileDescr
            ld (v_foundDescrPtr),hl
            call copyDescriptor
    IFDEF DOS_SPLIT_BUILD
            ld hl,(v_tmpFileDescr+FILE_DESCRIPTOR.firstClusterHi)
            ld a,h
            or l
            jp z,fileFind_highClusterOk
            call dosCorrupt
            jp fileFind_returnError
fileFind_highClusterOk:
    ENDIF
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
