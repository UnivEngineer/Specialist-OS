; HL=имя, C=1 запись / иначе чтение, B=новые атрибуты.
; Выход A=атрибуты, CF=ошибка; BC/DE/HL сохранены.
fatFileGetSetAttr:
            call fileFind
            ret c
            push hl
            push bc
            push de
            ld a,(v_tmpFileDescr+FILE_DESCRIPTOR.attrib)
            dec c
            jp nz,fatAttrExit
            ld a,b
            and 0D8h
            jp nz,fatAttrBad
            call markDirSectorAsModified
            jp c,fatAttrReturn
            ld a,(v_cachedDescrPtr)
            ld l,a
            ld de,FILE_DESCRIPTOR.attrib
            add hl,de
            ld (hl),b
            call saveSectorCache
            jp c,fatAttrReturn
            call fatFinishWrite
            jp c,fatAttrReturn
            ld a,b
fatAttrExit:
            or a
fatAttrReturn:
            jp popDBH_ret
fatAttrBad:
            ld a,DOS_ERR_ARGUMENT
            call dosSetError
            jp popDBH_ret
