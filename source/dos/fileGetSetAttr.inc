; HL=имя 8+3 в текущем каталоге, C=1 запись / иначе чтение, B=атрибуты.
; Меняются только R/H/S/A (27h), DIR сохраняется. Биты 6/7 и VOLUME запрещены.
; Выход A=атрибуты, CF=ошибка; BC/DE/HL сохранены.

;----------------------------------------------------------------------------
; MXOS
;
; Получить или изменить атрибуты записи каталога FAT.
;
; вход:
;   hl = короткое имя. Для записи c = 1, b = новые атрибуты. Любое другое значение c
;   запрашивает чтение.
;
; выход:
;   При cf = 0 регистр a содержит атрибуты. Изменяемые биты — 27h, бит каталога
;   сохраняется. При недопустимых битах или ошибке чтения/записи cf = 1. bc, de и hl
;   сохранены.
;----------------------------------------------------------------------------

fatFileGetSetAttr:
            call fileFindAny
            ret c
            push hl
            push bc
            push de
            ld a,(v_tmpFileDescr+FILE_DESCRIPTOR.attrib)
            dec c
            jp nz,fatAttrExit
            ld a,b
            and 0C8h
            jp nz,fatAttrBad
            ld a,(v_tmpFileDescr+FILE_DESCRIPTOR.attrib)
            and 010h
            ld c,a
            ld a,b
            and 027h
            or c
            ld b,a
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
