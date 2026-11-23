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
cacheData        = DOS_BANK_CACHE_TABLE
partialCacheData = DOS_PARTIAL_RECORD

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

;----------------------------------------------------------------------------
; MXOS
;
; Инвалидировать строки кэша заданного диска.
;
; вход:
;   a = номер диска. Вызов предназначен для отбрасывания старого содержимого после
;   внешней смены данных.
;
; выход:
;   Головы и продолжения соответствующих строк освобождены. af, bc, de и hl сохранены.
;----------------------------------------------------------------------------

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

;----------------------------------------------------------------------------
; MXOS
;
; Перевести индекс гранулы в адрес её описателя.
;
; вход:
;   a = индекс в пределах CACHE_GRANULES.
;
; выход:
;   hl = cacheData + 8 × индекс, bc = cacheData. a и de сохранены, cf определяется
;   последним сложением адресов.
;----------------------------------------------------------------------------

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
        jp m,cacheVictimNext  ; FAT12 удерживает первую половину разорванной ячейки.
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
;
; Повторное обращение к уже найденному сектору кэша.
;
; вход:
;   hl = адрес действующего головного описателя FAT_CACHE_DATA.
;   b = 0 для чтения, 1 для изменения сектора.
;
; выход:
;   hl = начало буфера. Установлен признак повторного обращения,
;   при b = 1 сектор помечен для записи. cf = 0, bc и de сохранены.
;   a и остальные флаги меняются.
;----------------------------------------------------------------------------

cacheTouchSlot:
        push de
        push bc
        jp cff_SectorFound

;----------------------------------------------------------------------------
; MXOS
;
; Получение адреса логического сектора через общий кэш.
;
; вход:
;   de = номер логического сектора текущего диска.
;   a = 0 для чтения, 1 для изменения сектора.
;
; выход:
;   cf = 0: hl = начало буфера. При записи сектор помечен как изменённый.
;   cf = 1: a = код ошибки кэша, hl непригоден для доступа.
;   bc, de сохраняются. Адрес действителен до вытеснения или сброса строки.
;
; 2022-02-08 Разработано SpaceEngineer
;----------------------------------------------------------------------------

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
;
; Получение адреса двухбайтной ячейки первой копии FAT16.
;
; вход:
;   de = номер ячейки. Геометрия выбранного тома задана в v_diskInfo.
;   a = 0 для чтения, 1 для изменения ячейки.
;
; выход:
;   cf = 0: hl указывает на младший байт ячейки в секторе кэша.
;   cf = 1: a = код ошибки кэша. Адрес в hl использовать нельзя.
;   bc сохраняется, de изменяется. Адрес действителен до вытеснения строки.
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
;
; Поиск сектора в общем кэше с загрузкой при промахе.
;
; вход:
;   hl = логический номер сектора выбранного диска.
;   b = 0 для чтения, 1 для изменения сектора.
;
; выход:
;   cf = 0: hl = начало буфера, при b = 1 сектор помечен для записи.
;   cf = 1: a = код ошибки чтения или записи при вытеснении.
;   bc и de сохраняются. Адрес в hl действителен до вытеснения строки.
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
        call cacheRememberHot
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

; HL=найденная голова; BC/DE/HL сохранены, Z=1, CF=0.
cacheRememberHot:
        push de
        push bc
        push hl
        ld a,(v_cacheHotNext)
        ld e,a
        ld d,0
        ld hl,v_cacheHotSlots
        add hl,de
        pop de
        ld (hl),e
        inc hl
        ld (hl),d
        ld a,(v_cacheHotNext)
        add a,2
        and 6
        ld (v_cacheHotNext),a
        ex de,hl
        pop bc
        pop de
        xor a
        ret
;----------------------------------------------------------------------------
; MXOS
;
; Проверить последние использованные головы кэша на совпадение сектора.
;
; вход:
;   de = логический номер сектора, выбран профиль текущего диска.
;
; выход:
;   При cf = 0 hl указывает на голову совпавшей строки. При cf = 1 среди последних
;   голов совпадения нет. de сохранён, af и bc рабочие.
;----------------------------------------------------------------------------

cacheProbeHot:
        ld hl,v_cacheHotSlots
        ld b,4
cacheHotLoop:
        push hl
        ld a,(hl)
        inc hl
        ld h,(hl)
        ld l,a
        call cacheSlotMatches
        pop hl
        jp z,cacheHotFound
        inc hl
        inc hl
        dec b
        jp nz,cacheHotLoop
        scf
        ret
cacheHotFound:
        ld a,(hl)
        inc hl
        ld h,(hl)
        ld l,a
        or a
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
;
; Чтение сектора в буфер, назначенный описателю.
;
; вход:
;   hl = адрес FAT_CACHE_DATA с заполненным полем address.
;   de = логический номер сектора на текущем диске.
;
; выход:
;   cf = 0: в буфере прочитанный сектор. cf = 1: ошибка ввода-вывода.
;   Номер записывается в описатель до чтения. При отказе вызывающий
;   должен отбросить строку. hl и de сохранены, b изменён на 2.
;   a и остальные флаги меняются.
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
;
; Запись изменённого сектора, включая вторую копию FAT при её наличии.
;
; вход:
;   hl = адрес описателя FAT_CACHE_DATA. Для изменённого сектора должен
;   быть выбран его диск. Чистый сектор пропускается.
;
; выход:
;   cf = 0: isModified = 0. cf = 1: a = код ошибки DOS, fatWriteError = 1,
;   isModified остаётся установленным для повторной записи.
;   hl и de сохраняются, b при записи загружается единицей.
;   a при успехе и остальные флаги меняются.
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
        ld      (fatWriteError), a
        call    dosIoError

ssfc_Exit:
        ; Восстанавливаем регситры и выходим
        pop     de
        pop     hl
        ret

;----------------------------------------------------------------------------
; MXOS
;
; Сброс изменённых секторов общего кэша и буфера неполного сектора.
;
; вход:
;   Активен драйвер диска, которому принадлежат изменённые секторы.
;
; выход:
;   cf = 0: a = 0, изменённых секторов не осталось.
;   cf = 1: a = код ошибки DOS. Обход остановлен на первом отказе,
;   успешно записанные строки уже чистые. hl и de сохраняются.
;   При записи единственной изменённой строки b может измениться.
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
