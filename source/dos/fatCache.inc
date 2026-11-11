;----------------------------------------------------------------------------
; MXOS
; Кэш секторов fat
;
; 2022-02-08 Разработано SpaceEngineer
;----------------------------------------------------------------------------

    STRUCT  FAT_CACHE_DATA          ; размер структуры 8 байт
number:         DW  0FFFFh          ; номер сектора
address:        DW  FAT_CACHE_ADDR  ; адрес закешированного сектора в ОЗУ
reference:      DW  0               ; младший байт — второй шанс алгоритма CLOCK
isModified:     DB  0               ; модифицирован ли сектор
tag:            DB  0FFh            ; диск/размер головы либо индекс продолжения
    ENDS

; Общий пул с гранулой 128 байт: сектор занимает 1, 2 или 4 гранулы.
; tag головы = (sectorShift-7)*8 + диск; 80h..BFh — индекс головы
; занятого продолжения; FFh — свободная гранула. Неактивные диски чистые:
; fileSelectDrive выполняет flush до смены физического драйвера.
cacheData = DOS_BANK_CACHE_TABLE
partialCacheData = cacheData + CACHE_GRANULES * FAT_CACHE_DATA
v_cacheTable: DW cacheData
v_cachePartial: DW partialCacheData
v_cacheBuffer: DW FAT_CACHE_ADDR
v_cachePartialBuffer: DW DOS_BANK_PARTIAL_ADDR
v_cacheLastSlot: DW cacheData
v_cacheLastTag: DB 0FFh
v_cacheProbeCount: DB 0
v_cacheMaxSector: DW 0
v_cacheStride: DW 128
v_cacheCount: DB CACHE_GRANULES
v_cacheUsed: DB 0
v_cacheDirty: DB 0 ; 0=чисто, 1=одна строка, 2=несколько
v_cacheDirtySlot: DW 0
v_cacheDrive: DB 0FFh
v_cacheShift: DB 0
v_cacheTag: DB 0FFh
v_cacheSpan: DB 1
v_cacheMask: DB 0FFh
v_cacheGeometries: BLOCK 8,0
v_cacheMaxByDrive: BLOCK 16,0
v_cacheClock: DB 0
v_allocIndex: DB 0
v_allocCursor: DB 0
v_allocEnd: DB 0
v_invalidateDrive: DB 0

resetAllSectorCaches:
        ld hl,v_cacheGeometries
        ld b,24
        xor a
cacheResetGeometry:
        ld (hl),a
        inc hl
        dec b
        jp nz,cacheResetGeometry
        ld (v_cacheShift),a
        dec a
        ld (v_cacheDrive),a
        ld (v_cacheTag),a
        jp resetSectorCache

; Строки остаются в пуле. Верхняя оценка LBA хранится отдельно для каждого
; диска: загруженный заголовок программы на A: не замедляет поток B:.
selectCacheForDrive:
        push af
        push de
        push hl
        ld a,(v_cacheDrive)
        cp 8
        jp nc,cacheSelectBound
        add a,a
        ld e,a
        ld d,0
        ld hl,v_cacheMaxByDrive
        add hl,de
        ex de,hl
        ld hl,(v_cacheMaxSector)
        ex de,hl
        ld (hl),e
        inc hl
        ld (hl),d
cacheSelectBound:
        ld a,(v_drive)
        ld (v_cacheDrive),a
        add a,a
        ld e,a
        ld d,0
        ld hl,v_cacheMaxByDrive
        add hl,de
        ld e,(hl)
        inc hl
        ld d,(hl)
        ex de,hl
        ld (v_cacheMaxSector),hl
        pop hl
        pop de
        pop af
        ret

; A=диск. Явный remount/смена драйвера инвалидируют только его строки.
; Вызывающий код предварительно сохраняет dirty прежним драйвером.
invalidateDriveCache:
        push af
        push bc
        push de
        push hl
        ld (v_invalidateDrive),a
        ld hl,cacheData+7
        ld c,CACHE_GRANULES
cacheInvalidateLoop:
        ld a,(hl)
        or a
        jp m,cacheInvalidateNext
        and 7
        ld b,a
        ld a,(v_invalidateDrive)
        cp b
        jp nz,cacheInvalidateNext
        push hl
        ld de,-7
        add hl,de
        call cacheInvalidateHead
        pop hl
cacheInvalidateNext:
        ld de,FAT_CACHE_DATA
        add hl,de
        dec c
        jp nz,cacheInvalidateLoop
        ld hl,partialCacheData+7
        ld a,(hl)
        and 7
        ld b,a
        ld a,(v_invalidateDrive)
        cp b
        jp nz,cacheInvalidateDone
        ld (hl),0FFh
        dec hl
        ld (hl),0
        ld hl,0FFFFh
        ld (partialCacheData),hl
cacheInvalidateDone:
        pop hl
        pop de
        pop bc
        pop af
        ret

; Полный сброс нужен только при первичной установке DOS.
resetSectorCache:
        xor a
        ld (v_cacheUsed),a
        ld (v_cacheDirty),a
        ld (v_cacheClock),a
        ld (v_cacheProbeCount),a
        ld hl,0
        ld (v_cacheMaxSector),hl
        ld hl,cacheData
        ld (v_cacheLastSlot),hl
        ld de,FAT_CACHE_ADDR
        ld b,CACHE_GRANULES
cacheResetSlot:
        ld (hl),0FFh
        inc hl
        ld (hl),0FFh
        inc hl
        ld (hl),e
        inc hl
        ld (hl),d
        inc hl
        xor a
        ld (hl),a
        inc hl
        ld (hl),a
        inc hl
        ld (hl),a
        inc hl
        ld (hl),0FFh
        inc hl
        push hl
        ld hl,128
        add hl,de
        ex de,hl
        pop hl
        dec b
        jp nz,cacheResetSlot
        ld hl,0FFFFh
        ld (partialCacheData),hl
        ld hl,DOS_BANK_PARTIAL_ADDR
        ld (partialCacheData+2),hl
        xor a
        ld (partialCacheData+6),a
        dec a
        ld (partialCacheData+7),a
        ret

; HL=голова: инвалидировать её и продолжения. BC/DE сохраняются.
cacheInvalidateHead:
        push bc
        push de
        push hl
        ld de,7
        add hl,de
        ld a,(hl)
        rrca
        rrca
        rrca
        and 3
        ld b,1
        or a
        jp z,cacheInvalidateSpan
        ld b,2
        dec a
        jp z,cacheInvalidateSpan
        ld b,4
cacheInvalidateSpan:
        pop hl
cacheInvalidateCell:
        ld (hl),0FFh
        inc hl
        ld (hl),0FFh
        ld de,5
        add hl,de
        ld (hl),0
        inc hl
        ld (hl),0FFh
        inc hl
        dec b
        jp nz,cacheInvalidateCell
        pop de
        pop bc
        ret

; A=индекс гранулы -> HL=описатель; BC разрушается.
cacheIndexToSlot:
        ld l,a
        ld h,0
        add hl,hl
        add hl,hl
        add hl,hl
        ld bc,cacheData
        add hl,bc
        ret

; CLOCK: занятая голова получает второй шанс после попадания. В отличие
; от поиска минимального возраста, вытеснение не сканирует весь пул каждый
; раз. Продолжения пропускаются; освобождаются целые пересекающие головы.
cacheAllocateSector:
        push de
cacheVictimLoop:
        ld a,(v_cacheClock)
        call cacheIndexToSlot
        ld bc,7
        add hl,bc
        ld a,(hl)
        cp 0FFh
        jp z,cacheVictimFound
        or a
        jp m,cacheVictimNext
        dec hl
        dec hl
        dec hl
        ld a,(hl)
        or a
        jp z,cacheVictimFound
        ld (hl),0
cacheVictimNext:
        ld a,(v_cacheClock)
        inc a
        and CACHE_GRANULES-1
        ld (v_cacheClock),a
        jp cacheVictimLoop
cacheVictimFound:
        ld a,(v_cacheClock)
        ld hl,v_cacheMask
        and (hl)
        ld (v_allocIndex),a
        ld (v_allocCursor),a
        ld b,a
        ld a,(v_cacheSpan)
        add a,b
        ld (v_allocEnd),a
cacheAllocateLoop:
        ld a,(v_allocCursor)
        call cacheIndexToSlot
        push hl
        ld bc,7
        add hl,bc
        ld a,(hl)
        cp 0FFh
        pop hl
        jp z,cacheAllocateNext
        or a
        jp p,cacheAllocateHead
        and 03Fh
        call cacheIndexToSlot
cacheAllocateHead:
        call saveSectorFromCache
        jp c,cacheAllocateError
        call cacheInvalidateHead
cacheAllocateNext:
        ld a,(v_allocCursor)
        inc a
        ld (v_allocCursor),a
        ld hl,v_allocEnd
        cp (hl)
        jp nz,cacheAllocateLoop
        ld a,(v_allocEnd)
        and CACHE_GRANULES-1
        ld (v_cacheClock),a
        ld a,(v_allocEnd)
        ld hl,v_cacheUsed
        cp (hl)
        jp c,cacheAllocated
        ld (hl),a
cacheAllocated:
        call cacheUpdateProbeCount
        ld a,(v_allocIndex)
        call cacheIndexToSlot
        pop de
        or a
        ret
cacheAllocateError:
        pop de
        ret

; HL=успешно загруженная голова: опубликовать tag и занять продолжения.
cachePublishSlot:
        push hl
        ld bc,7
        add hl,bc
        ld a,(v_cacheTag)
        ld (hl),a
        ld a,(v_allocIndex)
        or 080h
        ld c,a
        ld a,(v_cacheSpan)
        dec a
        jp z,cachePublishDone
        ld b,a
cachePublishNext:
        ld de,FAT_CACHE_DATA
        add hl,de
        ld (hl),c
        dec b
        jp nz,cachePublishNext
cachePublishDone:
        pop hl
        ret

;----------------------------------------------------------------------------
; MXOS
; Получение адреса закешированного сектора в памяти.
; Если сектор не найден в кэше, он загружается с текущего накопителя.
;
; вход:
;   de = номер сектора
;   a  = 1 - установить флаг isModified сектора
;
; выход:
;   hl = адрес сектора в буфере
;
;   регистры de, bc сохраняются
;
; 2022-02-08 Разработано SpaceEngineer
;----------------------------------------------------------------------------

; HL=найденный слот, B=dirty. HL=буфер; BC/DE сохранены, AF разрушен.
cacheTouchSlot:
        push de
        push bc
        jp cff_SectorFound

getSectorAddressInCache:
        push    bc
        push    de
        ld      b, a    ; b = isModified
        ex      hl, de

        ; Находим сектор в кэше; если нету - кешируем
        ; вход:  hl = номер сектора на диске
        ; выход: hl = адрес сектора в ОЗУ
        call    cacheSector

        pop     de
        pop     bc
        ret

;----------------------------------------------------------------------------
; MXOS
; Получение адреса ячейки fat.
; Если сектор fat не найден в кэше, он загружается с текущего накопителя.
;
; вход:
;   de = номер ячейки fat
;   a  = 1 - установить флаг isModified сектора
;
; выход:
;   hl = адрес ячейки в буфере
;
; 2022-02-08 Разработано SpaceEngineer
;----------------------------------------------------------------------------

getFatCellAddress:
        push    bc
        ld      b, a ; b = isModified

        ; Сначала делим номер записи: удвоение большого номера переполняет HL.
        ld h,d
        ld l,e
        ld a,(v_diskInfo.sectorShift)
        dec a
        call splitSectorOffset
        ex de,hl
        add hl,hl
        push hl                 ; двухбайтное смещение внутри сектора
        ld hl,(v_diskInfo.resSectors)
        add hl,de
        ; Находим сектор в кэше; если нету - кешируем
        ; вход:  hl = номер сектора на диске
        ; выход: hl = адрес сектора в ОЗУ
        call    cacheSector

        ; Находим смещение ячейки fat в этом секторе
        pop     de      ; de = смещение ячейки в секторе
        jp c,   fatCellExit
        add     hl, de  ; hl = адрес ячейки в секторе

fatCellExit:
        pop     bc
        ret

;----------------------------------------------------------------------------
; MXOS
; Поиск сектора в кэше. Если сектор не найден, то он загружается с накопителя.
;
; вход:
;   hl = номер сектора
;   b  = флаг модификации
;
; выход:
;   hl = адрес начала сектора в памяти
;
; регистры de, bc сохраняются
;
; 2022-02-08 Разработано SpaceEngineer
;----------------------------------------------------------------------------

cacheSector:
    IFDEF FAT_CACHE_BENCH_SAMPLE
        ; Только измерительная сборка: RET заменяется на JP таймера DISKBEN.
        ; Частые отсчёты исключают потерю оборотов ВИ53 при попаданиях в кэш.
        call    cacheBenchmarkSample
    ENDIF
        ; Сохраняем регистры
        push    de
        push    bc

        ; Просматриваем массив структур cacheData в поисках сектора с номером hl
        ex      hl, de                                      ; de = номер для поиска
        call    streamCacheContains
        jp nc,  cff_SectorFound

        call cacheAllocateSector
        jp c,cff_AllocateError
        call loadSectorToCache
        jp c,cff_ReadError
        push de
        call cachePublishSlot
        pop de
        ; Безопасная верхняя оценка LBA метаданных. После вытеснения её можно
        ; оставить завышенной: это даст только лишний поиск, но не пропуск.
        push hl
        ld hl,(v_cacheMaxSector)
        call cmp_hl_de
        jp nc,cff_MaxReady
        ex de,hl
        ld (v_cacheMaxSector),hl
        ex de,hl
cff_MaxReady:
        pop hl

        ; Сектор найден или загружен, возвращаем его адрес
        ; Здесь hl = адрес элемента массива cacheData этого сектора
cff_SectorFound:
        ld (v_cacheLastSlot),hl
        ld a,(v_cacheTag)
        ld (v_cacheLastTag),a

        inc     hl
        inc     hl      ; теперь hl = адрес поля address

        ; Читаем адрес сектора в de
        ld      e, (hl)
        inc     hl
        ld      d, (hl) ; de = адрес сектора
        inc     hl      ; теперь hl = адрес поля accessTime

        ; Попадание даёт второй шанс CLOCK без счётчика времени/переполнения.
        ld      (hl),1
        inc     hl
        ld      (hl),0
        inc     hl

        ; Устанавливаем isModified, если в регистре b на входе в функцию был не ноль
        pop     bc      ; b = флаг модификации
        ld      a, (hl)
        or      b
        ld      (hl), a
        or a
        jp z,cff_Clean
        call cacheMarkDirty
cff_Clean:

        ; hl = адрес сектора
        ex      hl, de

        ; Восстанавливаем регистры и выходим
        pop     de
        ret

; При единственной dirty-строке flush не должен сканировать весь пул.
; Кандидат может уже быть сохранён при вытеснении; тогда переиспользуем его.
cacheMarkDirty:
        push hl
        push de
        ld a,(v_cacheDirty)
        cp 2
        jp z,cacheDirtyReady
        ld hl,(v_cacheLastSlot)
        ex de,hl
        or a
        jp z,cacheDirtySingle
        ld hl,(v_cacheDirtySlot)
        call cmp_hl_de
        jp z,cacheDirtyReady
        push de
        ld de,6
        add hl,de
        pop de
        ld a,(hl)
        or a
        jp z,cacheDirtySingle
        ld a,2
        ld (v_cacheDirty),a
        jp cacheDirtyReady
cacheDirtySingle:
        ex de,hl
        ld (v_cacheDirtySlot),hl
        ld a,1
        ld (v_cacheDirty),a
cacheDirtyReady:
        pop de
        pop hl
        or a
        ret

; DE=LBA, A=dirty. Только неполные файловые сектора; общий кэш проверяем
; первым, чтобы не создать две расходящиеся копии одного сектора.
; BC/DE сохраняются, HL=буфер, CF=ошибка.
cachePartialSector:
        push bc
        push de
        ld b,a
        call cachePartialInner
        pop de
        pop bc
        ret
cachePartialInner:
        push de
        push bc
        call streamCacheContains
        jp nc,cff_SectorFound
        ld hl,(v_cachePartial)
        push bc
        call saveSectorFromCache
        pop bc
        jp c,cachePartialWriteError
        push bc
        call loadSectorToCache
        pop bc
        jp c,cff_ReadError
        push hl
        ld bc,7
        add hl,bc
        ld a,(v_cacheTag)
        ld (hl),a
        pop hl
        jp cff_SectorFound
cachePartialWriteError:
        pop bc
        pop de
        jp dosIoError

cff_AllocateError:
        pop bc
        pop de
        jp dosIoError

cff_ReadError:
        ld (hl),0FFh
        inc hl
        ld (hl),0FFh
        pop bc
        pop de
        jp dosIoError

;----------------------------------------------------------------------------
; MXOS
; Загрузка сектора с накопителя в кэш
;
; вход:
;   hl = адрес структуры FAT_CACHE_DATA с описанием сектора
;   de = номер сектора
;
; регистры hl, de, bc сохраняются
;
; 2022-02-08 Разработано SpaceEngineer
;----------------------------------------------------------------------------

loadSectorToCache:
        push    hl

        ; Записываем номер сектора в структуру
        ld      (hl), e
        inc     hl
        ld      (hl), d
        inc     hl      ; теперь hl = адрес поля address
        push    de

        ; Читаем в de адрес загрузки сектора
        ld      e, (hl)
        inc     hl
        ld      d, (hl)
        inc     hl

        ; Загружаем сектор
        ; Номер сектора = номер сектора, т.к. fat начинается с начала диска
        ex      hl, de  ; hl = адрес буфера
        pop     de      ; de = номер сектора
        ld      b, 2    ; режим - чтение
        or      a
        call    logicalSectorIOChecked

        ; Восстанавливаем регситры и выходим
        pop     hl
        ret

;----------------------------------------------------------------------------
; MXOS
; Запись сектора из кэша на накопитель, если его isModified не ноль
;
; вход:
;   hl = адрес структуры FAT_CACHE_DATA с описанием сектора
;
;   регистры hl, de, bc сохраняются
;
; 2022-02-08 Разработано SpaceEngineer
;----------------------------------------------------------------------------

saveSectorFromCache:
        push    hl
        push    de

        ; Проверяем, был ли изменен сектор
        push    bc
        ld      bc, FAT_CACHE_DATA.isModified   ; bc = смещение поля isModified
        add     hl, bc      ; hl = адрес поля isModified
        pop     bc
        ld      a, (hl)     ; a = isModified
        or      a           ; a == 0?
        jp z,   ssfc_Exit   ; если isModified == 0, сектор не был изменен, и его не надо сохранять

        ; Dirty сохраняется до успешной записи.
        push    hl

        dec     hl  ; теперь hl = адрес поля accessTime (ст. байт)

        dec     hl
        dec     hl  ; теперь hl = адрес поля address (ст. байт)

        ; Читаем в de адрес загрузки сектора
        ld      d, (hl)
        dec     hl
        ld      e, (hl)
        push    de  ; сохраняем его в стек

        dec     hl  ; теперь hl = адрес поля number (ст. байт)

        ; Читаем в de номер сектора
        ld      d, (hl)
        dec     hl
        ld      e, (hl)

        ; Записываем сектор
        ; de = номер сектора
        pop     hl      ; hl = адрес загрузки сектора
        ld      b, 1    ; режим - запись
        call    fatCachedWrite
        pop     hl
        jp c,   ssfc_Failed
        ld      (hl), 0
        jp      ssfc_Exit
ssfc_Failed:
        ; Не все внешние драйверы устанавливают общий признак ошибки.
        ; Сохраняем отказ независимо от результатов следующих записей.
        ld      a, 1
        ld      (bios_upper.diskWriteErr), a
        call    dosIoError

ssfc_Exit:
        ; Восстанавливаем регситры и выходим
        pop     de
        pop     hl
        ret

;----------------------------------------------------------------------------
; MXOS
; Запись всех закешированных секторов на накопитель, если их isModified не ноль
;
; регистры hl, de, bc сохраняются
;
; 2022-02-08 Разработано SpaceEngineer
;----------------------------------------------------------------------------

saveSectorCache:
        ld a,(v_cacheDirty)
        or a
        ret z
        cp 1
        jp nz,saveSectorCacheMany
        push hl
        ld hl,(v_cacheDirtySlot)
        call saveSectorFromCache
        pop hl
        ret c
        xor a
        ld (v_cacheDirty),a
        ret
saveSectorCacheMany:
        ; Сохраняем регситры
        push    hl
        push    de
        push    bc

        ld      hl,(v_cacheTable)           ; hl = первый элемент в массиве cacheData
        ld      de, FAT_CACHE_DATA      ; de = размер элемента массива cacheData
        ld      a,(v_cacheCount)
        ld      c,a  ; счетчик цикла
saveSectorCacheLoop:
        call    saveSectorFromCache     ; записываем сектор на накопитель
        jp c,   saveSectorCacheExit
        add     hl, de
        dec     c
        jp nz,  saveSectorCacheLoop
        ld hl,(v_cachePartial)
        call saveSectorFromCache
        jp c,saveSectorCacheExit
        xor a
        ld (v_cacheDirty),a
saveSectorCacheExit:

        ; Восстанавливаем регситры и выходим
        pop     bc
        pop     de
        pop     hl
        ret

;----------------------------------------------------------------------------
; MXOS
; Вычисление номера сектора по номеру кластера
;
; вход:
;   de = номер кластера
;
; формула:
;   de = (de - 2) * v_diskInfo.sectorsPerCluster + v_diskInfo.dataStartSector
;
; выход:
;   de = номер сектора
;
;   регистры bc, hl сохраняются
;
; 2022-02-14 Разработано SpaceEngineer
;----------------------------------------------------------------------------

fatClusterToSector:
        push    hl

        dec     de
        dec     de          ; de -= 2 (нумерация кластеров начинается с 2)

        ld a,(v_diskInfo.sectorsPerCluster)
        dec a
        jp z,fatClusterLinear
        ; BPB проверен при монтировании: множитель — степень двойки.
        ex      de, hl
        inc a
fatClusterShift:
        rrca
        add hl,hl
        cp 1
        jp nz,fatClusterShift
        ex      hl, de

fatClusterLinear:
        ld      hl, (v_diskInfo.dataStartSector)
        add     hl, de
        ex      hl, de

        pop     hl
        ret

;----------------------------------------------------------------------------
; MXOS
; Начало цикла по файлам в каталоге
;
; выход:
;   de = номер первого сектора каталога
;   с  = количество секторов каталога (мл. байт)
;
;   остальные регистры сохраняются
;
; 2022-02-14 Разработано SpaceEngineer
;----------------------------------------------------------------------------

;----------------------------------------------------------------------------
; MXOS
; Установка сектору каталога флага isModified
;
; вход:
;  v_cachedSector - номер сектора
;
; выход:
;   bc = системное время
;
;   регистр bc сохраняется
;
; 2022-02-08 Разработано SpaceEngineer
;----------------------------------------------------------------------------

markDirSectorAsModified:
        ld      hl, (v_cachedSector)
        ex      hl, de  ; de = номер сектора
        ld      a, 1    ; флаг - сектор будет изменен

        ; Загрузка сектора в кэш; если сектор уже в кэше,
        ; функция просто найдет его и изменит флаг
        call    getSectorAddressInCache
        ret c
        push de
        ex de,hl
        ld hl,(v_cachedDescrOffset)
        add hl,de
        ld (v_cachedDescrPtr),hl
        pop de
        ret

;----------------------------------------------------------------------------
; MXOS
; Сравнение слова по адресу hl и de
; Псевдокод: cp [hl], de
;
; выход:
;   флаги c, z
;
; 2022-02-08 Разработано SpaceEngineer
;----------------------------------------------------------------------------

cmp_m_de:
        inc   hl
        ld    a, (hl)
        dec   hl
        cp    d
        ret   c
        ret   nz
        ld    a, (hl)
        cp    e
        ret
    IFDEF FAT_CACHE_BENCH_SAMPLE
cacheBenchmarkSample:
        ret
        nop
        nop
    ENDIF
