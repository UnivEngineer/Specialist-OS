;----------------------------------------------------------------------------
; MXOS
; Кэш секторов fat
;
; 2022-02-08 Разработано SpaceEngineer
;----------------------------------------------------------------------------

    STRUCT  FAT_CACHE_DATA          ; размер структуры 8 байт
number:         DW  0FFFFh          ; номер сектора
address:        DW  FAT_CACHE_ADDR  ; адрес закешированного сектора в ОЗУ
accessTime:     DW  0               ; последнее время обращения к сектору
isModified:     DB  0               ; модифицирован ли сектор
_align:         DB  0               ; для выравнивания
    ENDS

; Массив из FAT_CACHE_CAPACITY структур длиной FAT_CACHE_DATA, инициализирован нулями
    IFDEF FAT_CACHE_TABLE_ADDR
; Экспериментальная таблица слотов вне резидентного кода DOS.
cacheData = FAT_CACHE_TABLE_ADDR
    ASSERT cacheData + (FAT_CACHE_SIZE / 128) * FAT_CACHE_DATA <= FAT_CACHE_ADDR
    ELSE
cacheData:      BLOCK   (FAT_CACHE_SIZE / 128) * FAT_CACHE_DATA, 0
    ENDIF

;----------------------------------------------------------------------------
; MXOS
; Инициализация кэша fat
;
; регисты НЕ сохраняются!
;
; 2022-02-08 Разработано SpaceEngineer
;----------------------------------------------------------------------------

resetSectorCache:
        ; bc = системное время
        call    getSystemTime

        ; В цикле записываем начальные значения во поля address
        ld      de, FAT_CACHE_ADDR      ; адрес буфера секторов
        ld      hl, cacheData           ; адрес первого элемента в массиве
        ld      a,  (v_cacheCount)  ; счетчик цикла

resetCacheLoop:
        ; Поле number = 0FFFFh
        ld      (hl), 0FFh
        inc     hl
        ld      (hl), 0FFh
        inc     hl
        ; Поле address = de
        ld      (hl), e
        inc     hl
        ld      (hl), d
        inc     hl
        ; Поле accessTime = bc
        ld      (hl), c
        inc     hl
        ld      (hl), b
        inc     hl
        ; Поле isModified = 0
        ld      (hl), 0
        inc     hl
        ; Поле isLoaded = 0
        ld      (hl), 0
        inc     hl

        ; de += SECTOR_SIZE
        push hl
        ld hl,(v_cacheStride)
        add hl,de
        ex de,hl
        pop hl

        ; Повтор чикла
        dec     a
        jp nz,  resetCacheLoop

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
        ld      hl, cacheData + FAT_CACHE_DATA.number       ; hl = адрес поля number в первом элементе массива
        ld      a,(v_cacheCount)
        ld      c,a                      ; счетчик цикла
cff_FindSector:
        call    cmp_m_de            ; сравниваем [hl] и de
        jp z,   cff_SectorFound     ; если равно, значит есть такой сектор в кэше
        push    bc                  ; иначе переходим на следущий элемент массива
        ld      bc, FAT_CACHE_DATA  ; bc = размер структуры FAT_CACHE_DATA
        add     hl, bc
        pop     bc
        dec     c                   ; повтор цикла
        jp      nz, cff_FindSector

        ; Не не нашли сектор в массиве, загружаем его с накопителя на место наиболее давно использовавашегося
        push    de  ; de = номер сектора, который надо загрузить

        ; Ищем наиболее давно использовавашийся сектор (т.е. у которого самый маленький accessTime)
        ld      de, 0FFFFh                                  ; de = максимальный найденый accessTime сектора
        ld      hl, cacheData + FAT_CACHE_DATA.accessTime   ; hl = адрес поля accessTime в первом элементе массива
        ld      a,(v_cacheCount)
        ld      c,a
        ld      b,0                      ; b  = 0, c = счетчик цикла
cff_FindOldestSector:
        call    cmp_m_de            ; сравниваем [hl] и de
        jp nc,  cff_next            ; переход, если [hl] >= de
        ld      e, (hl)             ; тут [hl] < de, запоминаем accessTime сектора в de
        inc     hl
        ld      d, (hl)
        dec     hl
        ld      b, c                ; запоминаем итерацию цикла сектора в b
cff_next:
        push    bc                  ; переходим на следущий элемент массива
        ld      bc, FAT_CACHE_DATA  ; bc = размер структуры FAT_CACHE_DATA
        add     hl, bc
        pop     bc
        dec     c                   ; повтор цикла
        jp      nz, cff_FindOldestSector

        ; Нашли наиболее давно использовавашийся сектор
        ld      a, (v_cacheCount)
        sub     b       ; a = индекс найденного сектора
    IF FAT_CACHE_CAPACITY > 32
        ; Смещение таблицы больше байта: HL = индекс * 8.
        ld      l, a
        ld      h, 0
        add     hl, hl
        add     hl, hl
        add     hl, hl
        ld      bc, cacheData
        add     hl, bc
    ELSE
        rla
        rla
        rla             ; a = a * 8 (размер структуры FAT_CACHE_DATA)
        ld      c, a
        ld      b, 0    ; bc = смещение сектора в массиве cacheData
        ld      hl, cacheData
        add     hl, bc  ; hl = адрес элемента массива
    ENDIF

        ; Прежде чем загрузить новый сектор, старый надо сохранить, если его isModified не ноль
        call    saveSectorFromCache    ; записываем сектор на накопитель

        jp c,   cff_WriteError

        ; Загружаем новый сектор на место старого
        pop     de              ; de = номер сектора, который надо загрузить
        call    loadSectorToCache    ; загружаем сектор с накопителя
        jp c,   cff_ReadError

        ; Сектор найден или загружен, возвращаем его адрес
        ; Здесь hl = адрес элемента массива cacheData этого сектора
cff_SectorFound:

        inc     hl
        inc     hl      ; теперь hl = адрес поля address

        ; Читаем адрес сектора в de
        ld      e, (hl)
        inc     hl
        ld      d, (hl) ; de = адрес сектора
        inc     hl      ; теперь hl = адрес поля accessTime

        ; Уcтанавливаем accessTime сектора
        call    getSystemTime;  bc = системное время

        ld      (hl), c
        inc     hl
        ld      (hl), b ; de = адрес сектора
        inc     hl      ; теперь hl = адрес поля isModified

        ; Устанавливаем isModified, если в регистре b на входе в функцию был не ноль
        pop     bc      ; b = флаг модификации
        ld      a, (hl)
        or      b
        ld      (hl), a

        ; hl = адрес сектора
        ex      hl, de

        ; Восстанавливаем регистры и выходим
        pop     de
        ret

cff_WriteError:
        pop de
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
        call    logicalSectorIO

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
        ld      (bios_vars.diskWriteErr), a
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
        ; Сохраняем регситры
        push    hl
        push    de
        push    bc

        ld      hl, cacheData           ; hl = первый элемент в массиве cacheData
        ld      de, FAT_CACHE_DATA      ; de = размер элемента массива cacheData
        ld      a,(v_cacheCount)
        ld      c,a  ; счетчик цикла
saveSectorCacheLoop:
        call    saveSectorFromCache     ; записываем сектор на накопитель
        jp c,   saveSectorCacheExit
        add     hl, de
        dec     c
        jp nz,  saveSectorCacheLoop
        or      a
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

        ld      hl, (v_diskInfo.sectorsPerCluster)
        ld a,l
        dec a
        jp z,fatClusterLinear
        call    mul_hl_de   ; hl = de * BPB_SecPerClus
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
; Получение текущего системного времени
;
; В "Специалисте" нет часов реального времени (RTC), поэтому системное время
; - это просто счетчик, увеличивающийся каждый раз при обращении. В будущем
; можно будет сделать вермя через таймер ВИ53, или через драйвер RTC, при его
; наличии в системе.
;
; выход:
;   bc = системное время
;
; все остальные регистры сохраняеюся
;
; 2022-02-08 Разработано SpaceEngineer
;----------------------------------------------------------------------------

getSystemTime:
        push    hl
        ld      hl, (v_fakeSystemTime)
        inc     hl
        ld      (v_fakeSystemTime), hl
        ld      b, h
        ld      c, l
        pop     hl
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
