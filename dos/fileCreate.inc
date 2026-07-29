;----------------------------------------------------------------------------
; MXOS
; Создать файл
;
; На входе
;  hl - адрес дескриптора файла в формате FAT (имя, фтрибуты, размер и т.д.)
;
; На выходе
;  сf - ошибка
;
; TODO: В случае нехватки места на диске, на нем остается неполный файл.
; Надо удалить то, что записали.
;
; 2013-12-12 Дизассемблировано vinxru
; 2022-02-11 Переработано SpaceEngineer
;----------------------------------------------------------------------------

; Внутренний вход для потокового копирования.
; HL=FILE_DESCRIPTOR, DE=число 256-байтных кластеров.
; Создаёт запись каталога и FAT-цепочку, но не пишет данные.
fileCreatePrepare:
            ld      a, 1
            ld      (v_fileCreatePrepare), a
            ex      de, hl
            ld      (v_input_end), hl
            ex      de, hl
            jp      fileCreateEntry

fileCreate: ; Сохраняем регистры
            xor     a
            ld      (v_fileCreatePrepare), a

fileCreateEntry:
            push    hl  ; [1]
            push    bc  ; [2]
            push    de  ; [3]

            ; Сохраняем адрес подготовленного дескриптора создаваемого файла
            ld      (v_newDescrPtr), hl

            ; Если файл существует, то удаляем его
            call    fileFind
            call nc,deleteFileInt

            ; de = номер первого сектора каталога
            ; с  = количество секторов каталога (мл. байт)
            call    fatBeginDirLoop

            ; Цикл по секторам каталога
fileCreate_dirLoop:
            ; Сохраняем номер сектора каталога
            push    de  ; [4]               ; de = номер сектора каталога

            ; Кэшируем сектор каталога
            xor     a                       ; флаг - сектор не будет меняться
            call    getSectorAddressInCache ; выход: hl = адрес сектора в буфере

            ; Кол-во дескрипторов файлов в одном секторе каталога (мл. байт)
            ld      a, (v_diskInfo.descrPerSector)
            ld      b, a

            ; Размер дескриптора
            ld      de, FILE_DESCR_SIZE

            ; Цикл по дескрипторам в закешированном секторе каталога
fileCreate_sectorLoop:
            ld      a, (hl)                 ; читаем байт из каталога
            inc     a                       ; если байт = 0FFh, значит запись свободна
            jp z,   fileCreate_placeFound   ; переход, если свободная запись найдена

            ; Вычисляем адрес следующего дескриптора
            add     hl, de

            ; Еще остались файлы в секторе каталога?
            dec     b
            jp nz,  fileCreate_sectorLoop

            ; Еще остались секторы каталога?
            ; Восстанавливаем номер сектора каталога
            pop     de  ; [4]   de = номер сектора каталога
            inc     de  ;       следующий сектор каталога

            dec     c
            jp nz,  fileCreate_dirLoop

            ;-----------------------------------------------------------
            ; Свободных записей нет

            ; Установка a=0, ZF=1
            xor     a           

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

            ; Сохраняем найденный в кэше адрес свободного дескриптора
            ld      (v_cachedDescrPtr), hl

            ; Сохраняем номер сектора каталога
            pop     de  ; [4]   de = номер сектора каталога
            ex      hl, de
            ld      (v_cachedSector), hl

            ; Находим первый свободный кластер
            call    fatFindClusterFirst ; de = первый свободный кластер
            jp c,   stc_popDBH_ret      ; если нет свободных кластеров - выходим с ошибкой

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
            ; Записываем дескриптор в каталог, пока сектор каталога все еще в кэше

            ; Ставим сектору каталога флаг isModified
            call    markDirSectorAsModified

            ; Копируем подготовленный дескриптор файла в закешированный сектор каталога
            ld      hl, (v_newDescrPtr)     ; адрес подготовленного дескриптора
            ex      hl, de
            ld      hl, (v_cachedDescrPtr)  ; адрес свободного дескриптора в кэше

            ; Копирование c байт из de в hl
            push    bc
            call    copyDescriptor
            pop     bc

            ld      a, (v_fileCreatePrepare)
            or      a
            jp nz,  fileCreate_allocate

            ;---------------------------------------------------------------------

		    ; Читаем адрес загрузки файла
            ; de = адрес загрузки файла
            ld      hl, (v_newDescrPtr)
            ld      bc, FILE_DESCRIPTOR.loadAddress
            add     hl, bc  ; hl = адрес поля loadAddress (мл. байт) в дескрипторе файла
            ld      e, (hl)
            inc     hl      ; hl = адрес поля loadAddress (ст. байт) в дескрипторе файла
            ld      d, (hl) ; de = адрес загрузки

            ; Сохраняем адрес загрузки в переменную v_input_start
            ex      de, hl
            ld      (v_input_start), hl ; v_input_start = адрес загрузки файла

            ; Читаем размер файла
            ; de = размер файла
            ld      hl, (v_newDescrPtr)
            ld      bc, FILE_DESCRIPTOR.size
            add     hl, bc  ; hl = адрес поля size (мл. байт) в дескрипторе файла
            ld      e, (hl)
            inc     hl      ; hl = адрес поля size (ст. байт) в дескрипторе файла
            ld      d, (hl)

            ; Сохраняем размер файла в переменную v_input_end
            ex      de, hl
            ld      (v_input_end), hl   ; v_input_end = размер файла

            ; Счётчик кластеров файла
            ld      c, h        ; c = число 256-байтных блоков (0 означает 256)
            inc     c
            ld      l, c
            ld      h, 0
            ld      a, c
            or      a
            jp nz,  fileCreate_countReady
            inc     h
fileCreate_countReady:
            ld      (v_input_end), hl

fileCreate_allocate:            ; Первый кластер файла в fat
            ld      hl, (v_fileFirstCluster)    ; hl = номер первого кластера файла
            ld      d, h
            ld      e, l                        ; de = hl - если файл состоит из одного кластера, ячейка fat будет указывать на саму себя

            jp      writeFirstCluster

; ---------------------------------------------------------------------------
; Записываем кластеры файла в таблицу fat на устройстве

allocClusters:
            call    fatFindClusterNext  ; de = следующий свободный кластер
            jp nc,  writeFirstCluster

fileCreate_allocFailed:
            ; Замыкаем уже созданную часть цепочки для безопасного удаления.
            ld      d, h
            ld      e, l
            call    fatWriteCluster
            jp      stc_popDBH_ret

writeFirstCluster:            ; записываем номер кластера в fat
            ; hl = номер ячейки fat
            ; de = номер кластера
            call    fatWriteCluster

            push    hl
            ld      hl, (v_input_end)
            dec     hl
            ld      (v_input_end), hl
            ld      a, h
            or      l
            pop     hl
            jp nz,  allocClusters       ; повтор цикла

            ; Конец, последний кластер файла указывает сам на себя
            ; hl == de
            call    fatWriteCluster

            ld      a, (v_fileCreatePrepare)
            or      a
            jp nz,  fileCreate_prepareDone

; ---------------------------------------------------------------------------
; Сохраняем файл на диск

            ; Номер первого кластера
            ld      hl, (v_fileFirstCluster)
            ex      hl, de

            ld      hl, (v_newDescrPtr)
            ld      bc, FILE_DESCRIPTOR.size + 1
            add     hl, bc
            ld      c, (hl)
            inc     c

            ld      hl, (v_input_start)
            ld      b, 1
            call    fileTransferClusters

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
            or      a               ; выходим с флагом cf = 0
            jp      popDBH_ret

v_fileCreatePrepare: DB 0

