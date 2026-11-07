;----------------------------------------------------------------------------
; MXOS
; Создать файл
;
; На входе
;  hl - адрес дескриптора файла в формате FAT (имя, фтрибуты, размер и т.д.)
;  de - адрес исходного буфера; размер в дескрипторе равен N
;
; На выходе
;  сf - ошибка
;
; При нехватке кластеров частичная запись и цепочка удаляются.
; Аппаратный отказ записи не обеспечивает транзакционный откат.
;
; 2013-12-12 Дизассемблировано vinxru
; 2022-02-11 Переработано SpaceEngineer
;----------------------------------------------------------------------------

; Внутренний вход для потокового копирования.
; HL=FILE_DESCRIPTOR с точной 32-битной длиной. DE — рабочий регистр.
; Создаёт запись каталога и FAT-цепочку, но не пишет данные.
fatFileCreatePrepare:
            ; Длину цепочки определяет точный размер, а не размер сектора вызывающего.
            call fatSizeToClusters
            ret c
            ld      a, 1
            ld      (v_fileCreatePrepare), a
            ex      de, hl
            ld      (v_input_end), hl
            ex      de, hl
            jp      fileCreateEntry

fatFileCreate: ; HL=дескриптор с точным размером, DE=буфер данных.
            ex de,hl
            ld (v_input_start),hl
            ex de,hl
            ; Сохраняем регистры
            xor     a
            ld      (v_fileCreatePrepare), a
            ; Обычный create читает единый адресный буфер; старшее слово
            ; длины разрешено только потоковому createPrepare.
            push hl
            push de
            ld de,FILE_DESCRIPTOR.size+2
            add hl,de
            ld a,(hl)
            inc hl
            or (hl)
            pop de
            pop hl
            jp z,fatCreateSmallSize
            ld a,DOS_ERR_ARGUMENT
            jp dosSetError
fatCreateSmallSize:

fileCreateEntry:
            push hl
            push de
            ld de,FILE_DESCRIPTOR.attrib
            add hl,de
            ld a,(hl)
            and 0D8h
            jp nz,fatCreateBadMetadata
            inc hl
            ld a,(hl)
            and 018h
            ld (hl),a
            ld de,FILE_DESCRIPTOR.firstClusterHi-FILE_DESCRIPTOR.ntReserved
            add hl,de
            xor a
            ld (hl),a
            inc hl
            ld (hl),a
            pop de
            pop hl
            jp fatCreateMetadataOk
fatCreateBadMetadata:
            pop de
            pop hl
            ld a,DOS_ERR_ARGUMENT
            jp dosSetError
fatCreateMetadataOk:
            xor     a
            ld      (bios_vars.diskWriteErr), a
            push    hl  ; [1]
            push    bc  ; [2]
            push    de  ; [3]

            ; Сохраняем адрес подготовленного дескриптора создаваемого файла
            ld      (v_newDescrPtr), hl

            ; Если файл существует, то удаляем его
            call    fileFindAny
            jp c,   fileCreate_noExisting
            ; Каталог нельзя заменить файлом. System допускает изменение содержимого.
            ld      a, (v_tmpFileDescr + FILE_DESCRIPTOR.attrib)
            and     011h
            jp nz,  fileCreate_accessDenied

            ; При замене сохраняем атрибуты существующего файла.
            ld      a, (v_tmpFileDescr + FILE_DESCRIPTOR.attrib)
            ld      hl, (v_newDescrPtr)
            ld      de, FILE_DESCRIPTOR.attrib
            add     hl, de
            ld      (hl), a
            call    deleteFileInt
            jp c,   popDBH_ret
            jp      fileCreate_ready
fileCreate_noExisting:

            ; Общий ограниченный курсор корня.
            ld a,(v_dosError)
            cp DOS_ERR_NOT_FOUND
            jp nz,stc_popDBH_ret
fileCreate_ready:
            xor a
            ld (v_dosError),a
            call fatDirStart
            jp c,popDBH_ret
fileCreate_dirLoop:
            call fatDirNext
            jp c,fileCreate_dirEnd
            ld a,(hl)
            or a
            jp z,fileCreate_placeFound
            cp 0E5h
            jp nz,fileCreate_dirLoop
            jp fileCreate_placeFound
fileCreate_dirEnd:
            or a
            jp nz,stc_popDBH_ret
            ; Корень FAT12/16 фиксирован и никогда не расширяется.
            ld hl,(v_directory)
            ld a,h
            or l
            ld a,DOS_ERR_DIR_FULL
            call z,dosSetError
            call nz,directoryExtendGate
            jp nc,fileCreate_placeFound
            jp popDBH_ret

stc_popDBH_ret:
            ; Установка флага CF=1
            scf

popDBH_ret: ; Восстановление регистров и выход
            pop     de  ; [3]
            pop     bc  ; [2]
            pop     hl  ; [1]
            ret

            ;-----------------------------------------------------------
            ; Свободная запись найдена

fileCreate_placeFound:
            call fatDirPrepareEntry
            jp c,popDBH_ret
            ; Сохраняем найденный в кэше адрес свободного дескриптора
            ld      (v_cachedDescrPtr), hl

            ; Сохраняем номер сектора каталога
            ; v_cachedSector уже установлен общим курсором.

            ; Находим первый свободный кластер
            ld hl,(v_newDescrPtr)
            ld de,FILE_DESCRIPTOR.size
            add hl,de
            ld b,4
            xor a
fileCreate_checkEmpty:
            or (hl)
            inc hl
            dec b
            jp nz,fileCreate_checkEmpty
            or a
            ld de,0
            jp z,fileCreate_firstReady
            call    fatFindClusterFirst ; de = первый свободный кластер
            jp c,   stc_popDBH_ret      ; если нет свободных кластеров - выходим с ошибкой

fileCreate_firstReady:
            ; Сохраняем его в переменную v_fileFirstCluster
            ex      hl, de
            ld      (v_fileFirstCluster), hl
            ex      hl, de

            ; Сохраняем номер первого кластера в дескриптор файла
            ; de = номер первого кластера
            ; hl = адрес дескриптора файла
            ld      hl, (v_newDescrPtr)
            ld      bc, FILE_DESCRIPTOR.firstCluster
            add     hl, bc  ; hl = адрес поля firstCluster (мл. байт) в дескрипторе файла
            ld      (hl), e
            inc     hl      ; hl = адрес поля firstCluster (ст. байт) в дескрипторе файла
            ld      (hl), d


            ;---------------------------------------------------------------------
            ; Поиск свободного кластера мог вытеснить сектор каталога из кэша.

            ; Ставим сектору каталога флаг isModified
            call    markDirSectorAsModified
            jp c,   popDBH_ret

            ; HL теперь указывает на актуальный буфер сектора каталога.
            ; При секторах по 256 байт младший байт старого указателя —
            ; смещение записи внутри сектора. Восстанавливаем адрес после
            ; возможного перемещения сектора в другой слот кэша.
            ld      (v_cachedDescrPtr), hl

            ; Копируем подготовленный дескриптор файла в закешированный сектор каталога
            ld      hl, (v_newDescrPtr)     ; адрес подготовленного дескриптора
            ex      hl, de
            ld      hl, (v_cachedDescrPtr)  ; адрес свободного дескриптора в кэше

            ; Копирование c байт из de в hl
            push    bc
            call    copyDescriptor
            ld      hl,(v_cachedDescrPtr)
            call    fatEncodeName
            pop     bc

            ld hl,(v_fileFirstCluster)
            ld a,h
            or l
            jp z,fileCreate_prepareDone
            ld      a, (v_fileCreatePrepare)
            or      a
            jp nz,  fileCreate_allocate

            ;---------------------------------------------------------------------

            ld hl,(v_newDescrPtr)
            call fatSizeToClusters
            jp c,popDBH_ret
            ex de,hl
            ld (v_input_end),hl

fileCreate_allocate:
            ld      hl,(v_fileFirstCluster)
writeFirstCluster:
            ; Хвост резервируется до поиска: полный диск не выдаст его повторно.
            push    hl
            ld      de,0FFFFh
            call    fatWriteCluster
            pop     hl
            jp c,   popDBH_ret
            push    hl
            ld      hl,(v_input_end)
            dec     hl
            ld      (v_input_end),hl
            ld      a,h
            or      l
            pop     hl
            jp z,   fileCreate_allocDone
allocClusters:
            call    fatFindClusterNext
            jp c,   fileCreate_allocFailed
            call    fatWriteCluster
            jp c,   popDBH_ret
            jp      writeFirstCluster
fileCreate_allocFailed:
            ld      a,(v_dosError)
            push    af
            ld      hl,(v_newDescrPtr)
            call    fileFind
            call nc,deleteFileInt
            call    saveSectorCache
            pop     af
            call    dosSetError
            jp      popDBH_ret
fileCreate_allocDone:
            ld      a, (v_fileCreatePrepare)
            or      a
            jp nz,  fileCreate_prepareDone

; ---------------------------------------------------------------------------
; Сохраняем файл на диск

            ; Номер первого кластера
            ld      hl, (v_fileFirstCluster)
            ex      hl, de

            push de
            ld hl,(v_newDescrPtr)
            ld bc,FILE_DESCRIPTOR.size
            add hl,bc
            ld e,(hl)
            inc hl
            ld d,(hl)
            ex de,hl
            pop de
            ld b,h
            ld c,l
            ld hl,0
            ld (v_streamOffset),hl
            ld hl,(v_input_start)
            ld a,1
            call fatStream
            jp c,popDBH_ret

; ---------------------------------------------------------------------------
; Расчет однобайтовой контрольной суммы
; Это нигде не используется даже в оригинальной ОС

;            ld      hl, (v_input_end)   ; hl = размер файла - 1 из v_input_end
;            inc     hl                  ; hl = размер файла
;            ex      de, hl              ; de = размер файла
;            ld      hl, (v_input_start) ; hl = адрес загрузки файла из v_input_start
;            ld      b, 0                ; тут будет накапливаться сумма
;fileCreateCRCLoop:
;            ld      a, d
;            or      e
;            jp z,   fileCreateCRCDone   ; если счетчик досчитал до нуля, выходим
;            ld  a,  (hl)                ; читаем байт файла
;            add     a, b                ; накапливаем сумму
;            ld      b, a
;            inc     hl                  ; следующий байт
;            dec     de                  ; уменьшаем счетчик
;            jp      fileCreateCRCLoop
;fileCreateCRCDone:
;            ; Сохраняем контрольную сумму в дескриптор файла, в поле ntReserved
;            ld      a, b                ; a = посчитанная контрольная сумма
;            ld      hl, (v_newDescrPtr)
;            ld      bc, FILE_DESCRIPTOR.ntReserved
;            add     hl, bc              ; hl = адрес поля ntReserved в дескрипторе файла
;            ld      (hl), a             ; сохраняем контрльную сумму

; ------------------------------------------------------------------------------------------------------------------------
; Сохраняем кэш и выходим

fileCreate_prepareDone:
            call    saveSectorCache ; сохраняем изменения в кэше на диск
            call    fatFinishWrite
            jp      popDBH_ret

v_fileCreatePrepare: DB 0


fileCreate_accessDenied:
            ld a,DOS_ERR_ACCESS
            call dosSetError
            jp popDBH_ret
