; Общий курсор корня. Начало не меняет BC/DE/HL, AF разрушается.
fatDirStart:
            xor a
            ld (v_dirIndex),a
            ld (v_dirIndex+1),a
            ld (v_dirRemaining),a
            push hl
            ld hl,(v_diskInfo.dirStartSector)
            dec hl
            ld (v_cachedSector),hl
            pop hl
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
            ld hl,(v_dirIndex)
            push hl
            ld hl,(v_diskInfo.rootDirMaxFiles)
            ex de,hl
            pop hl
            call cmp_hl_de
            jp nc,fatDirEnd
            push hl
            inc hl
            ld (v_dirIndex),hl
            pop hl
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
; Следующий обычный файл. 00h завершает каталог, E5h пропускается.
; Каталоги, метки и LFN пока не выдаются за файлы. Регистры как fatDirNext.
fatDirFile:
            call fatDirNext
            ret c
            ld a,(hl)
            or a
            jp z,fatDirEndFile
            cp 0E5h
            jp z,fatDirFile
            push hl
            push de
            ld de,FILE_DESCRIPTOR.attrib
            add hl,de
            ld a,(hl)
            and 018h
            pop de
            pop hl
            jp nz,fatDirFile
            or a
            ret
fatDirEndFile:
            scf
            ret
v_dirRemaining: DB 0
v_dirIndex: DW 0
v_cachedDescrOffset: DW 0

; HL=запись каталога. E5h в первой букве имени хранится как 05h.
; HL/BC/DE сохранены, AF разрушается.
fatEncodeName:
            ld a,(hl)
            cp 0E5h
            ret nz
            ld (hl),5
            ret
