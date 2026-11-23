;----------------------------------------------------------------------------
; MXOS
;
; Подсчёт свободных кластеров данных в первой копии FAT.
;
; вход:
;   Выбранный том и текущий каталог заданы в v_diskInfo и v_directory.
;
; выход:
;   cf = 0: a = 0, de = число свободных кластеров данных.
;   cf = 1: a = код ошибки тома, каталога или кэша. de не содержит итога.
;   bc, hl сохраняются. Обход переустанавливает курсор текущего каталога.
;----------------------------------------------------------------------------

fatGetDriveFreeSpace:
            call fatDirStart
            ret c
    IFDEF BANKED_DOS_BUILD
            ld a,(v_fat12)
            or a
            jp nz,f12Count
    ENDIF
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
; Сектор FAT запрашивается один раз, затем записи читаются линейно.
; Горячий путь занятой записи без границы сектора: 85 тактов 8080,
; свободной: ещё 5 тактов INC DE. При совпадении младшего байта конца
; добавляются 27 тактов проверки старшего (112/117). Два чтения FAT
; и одно/два чтения границы из RAM, без портов; смена сектора зависит от I/O.
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
;----------------------------------------------------------------------------
; MXOS
;
; Чтение очередного сектора FAT для подсчёта свободного места.
;
; вход:
;   v_freeSector = номер логического сектора на выбранном томе.
;
; выход:
;   cf = 0: hl = начало сектора в кэше, v_freeEnd = адрес сразу за ним.
;   cf = 1: a = код ошибки кэша, hl непригоден для чтения.
;   bc, de сохраняются. Указатель действителен до вытеснения сектора.
;----------------------------------------------------------------------------

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
