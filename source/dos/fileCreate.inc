; Создание записи и предварительное выделение FAT-цепочки.
; HL=FILE_DESCRIPTOR с точной 32-битной длиной; данные пишет FILEHANDLE/fatStream.
; Общий примитив FILEHANDLE, FILEOPS и MKDIR. Не является whole-file writer.
fatFileCreatePrepare:
            ; Длину цепочки определяет точный размер, а не размер сектора вызывающего.
            call fatSizeToClusters
            ret c
            ex      de, hl
            ld      (v_input_end), hl
            ex      de, hl
            jp      fileCreateEntry

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
            ld      (fatWriteError), a
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
fileCreate_prepareDone:
            call    saveSectorCache ; сохраняем изменения в кэше на диск
            call    fatFinishWrite
            jp      popDBH_ret



fileCreate_accessDenied:
            ld a,DOS_ERR_ACCESS
            call dosSetError
            jp popDBH_ret
