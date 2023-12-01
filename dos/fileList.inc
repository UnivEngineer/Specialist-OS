;+---------------------------------------------------------------------------
; MXOS
; Получить список файлов каталога.
; Функция копирует в буфер уомпактные описатели файлов (FILE_INFO).
;
; На входе
;  hl - адрес буфера
;  de - размер буфера (макс. количество описателей файлов)
;  bc - с какого дескриптора начинать заполнение буфера
;
; На выходе
;  de - сколько всего дескрипторов в каталоге
;
;  bc, hl - сохраняются
;
; 2013-12-12 Дизассемблировано vinxru
; 2022-02-11 Переработано SpaceEngineer
;----------------------------------------------------------------------------

fileList:   ; Если нет драйвера или диск не отформатирован -
            ; формируем пустой список и выходим
            ld      a, (v_diskInfo.isValid)
            cp      DISK_VALID
            jp z,   fileListOk

            ; В начало буфера помещаем символ 0FFh
            ld      (hl), 0FFh

            ; Возвращаем количество файлов в директории в de (ноль)
            ld      de, 0
            ret

fileListOk: ; Сохранение регистров
            push    bc  ; [1]
            push    hl  ; [2]
            
            ; В de - максимум файлов, но его надо еще ограничить размером каталога
            ld      hl, (v_diskInfo.rootDirMaxFiles)
            call    cmp_hl_de_2
            jp c,   fl_min
            ex      hl, de
fl_min:     ; Сохраняем максимум файлов
            ld      (v_dirMaxFiles), hl

            ; Сохраняем номер первого файла
            ld      h, b
            ld      l, c
            ld      (v_dirFirstFile), hl

            ; Обнуляем счетчики файлов
            ld      hl, 0
            ld      (v_dirListedFiles), hl
            ld      (v_dirTotalFiles),  hl

            pop     hl  ; [2]
            push    hl  ; [2]

            ; de = номер первого сектора каталога
            ; с  = количество секторов каталога (мл. байт)
            call    fatBeginDirLoop

            ; Цикл по секторам каталога
fileList_dirLoop:
            ; Сохраняем номер сектора каталога
            push    de  ; [3]               ; de = номер сектора каталога

            ; Кэшируем сектор каталога
            push    hl  ; [4]
            xor     a                       ; флаг - сектор не будет меняться
            call    getSectorAddressInCache ; выход: hl = адрес сектора в буфере
            ex      hl, de                  ; de = адрес сектора в буфере
            pop     hl  ; [4]

            ; Кол-во дескрипторов файлов в одном секторе каталога (мл. байт)
            ld      a, (v_diskInfo.descrPerSector)
            ld      b, a

            ; Цикл по дескрипторам в закешированном секторе каталога
fileList_sectorLoop:
            ; Если первый символ = FFh, дескриптор пустой
            ld      a, (de)
            inc     a
            jp z,   fileList_nextDescr

            push    hl  ; [4]
            push    de  ; [5]

            ; Увеличиваем счетчик общего количества файлов в каталоге
            ld      hl, (v_dirTotalFiles)
            inc     hl
            ld      (v_dirTotalFiles), hl

            ; Если не дошли до первого файла (v_dirTotalFiles < v_dirFirstFile),
            ; то продолжаем сканирование каталога без копирования дескриптора в буфер
            ex      hl, de
            ld      hl, (v_dirFirstFile)
            call    cmp_hl_de
            jp nc,  fileList_skipCopy

            ; Если буфер уже заполнен (v_dirListedFiles >= v_dirMaxFiles),
            ; то продолжаем сканирование каталога без копирования дескриптора в буфер
            ld      hl, (v_dirMaxFiles)
            ex      hl, de
            ld      hl, (v_dirListedFiles)
            call    cmp_hl_de
            jp nc,  fileList_skipCopy

            ; Увеличиваем счетчик скопированных дескрипторов
            inc     hl
            ld      (v_dirListedFiles), hl

            pop     de  ; [5]
            pop     hl  ; [4]

            ; Копируем дескриптор из de в hl
            push    bc  ; [4]
            ;call    copyDescriptor
            call    copyDescriptorCompact
            pop     bc  ; [4]

            ; copyDescriptor смещает de, поэтому код расчета
            ; адреса следующего дескриптора надо перепрыгнуть
            ;jp      fileList_next
            jp      fileList_nextDescr

fileList_skipCopy:
            pop     de  ; [5]
            pop     hl  ; [4]

fileList_nextDescr:
            ; Вычисляем адрес следующего дескриптора
            push    hl  ; [4]
            ld      hl, FILE_DESCR_SIZE
            add     hl, de
            ex      de, hl
            pop     hl  ; [4]

fileList_next:
            ; Еще остались файлы в секторе каталога?
            dec     b
            jp nz,  fileList_sectorLoop

            ; Еще остались секторы каталога?
            ; Восстанавливаем номер сектора каталога
            pop     de  ; [3]   de = номер сектора каталога
            inc     de  ;       следующий сектор каталога
            dec     c
            jp nz,  fileList_dirLoop

fileList_exit
            ; Завершение и выход
            ; В конец буфера помещаем символ 0FFh
            ld      (hl), 0FFh

            ; Возвращаем количество файлов в директории в de
            ld      hl, (v_dirTotalFiles)
            ex      hl, de

            pop     hl  ; [2]
            pop     bc  ; [1]
            ret

