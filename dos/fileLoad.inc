;+---------------------------------------------------------------------------
; MXOS
; Загрузить файл по указанному адресу
;
; На входе
;  hl - имя
;  de - адрес загрузки
;
; На выходе
;  bc, de, hl - сохраняются
;  v_foundDescrPtr - адрес копии дескриптора найденного файла
;  cf - ошибка
;
; 2013-12-12 Дизассемблировано vinxru
; 2022-02-11 Переработано SpaceEngineer
;----------------------------------------------------------------------------

fileLoad2:  ; Ищем файл с именем в hl
            call    fileFind
            ret c 

            ; Сохранить регистры
            push    hl
            push    bc
            push    de

            jp      fileLoadStep2

;----------------------------------------------------------------------------
; Загрузить файл по адресу указанному в заголовке
;
; На входе
;  hl - имя
;
; На выходе
;  bc, de, hl - сохраняются
;  v_foundDescrPtr - адрес копии дескриптора найденного файла
;  cf - ошибка
;----------------------------------------------------------------------------

fileLoad:   ; Ищем файл с именем в hl
            call    fileFind
            ret c 

            ; Сохраняем регистры
            push    hl
            push    bc
            push    de

            ; hl = указатель на адрес загрузки
            ld      hl, (v_foundDescrPtr)
            ld      de, FILE_DESCRIPTOR.loadAddress
            add     hl, de

            ; de = адрес загрузки
            ld      e, (hl)
            inc     hl
            ld      d, (hl)

fileLoadStep2:
            ; Сохраним адрес загрузки
            ex      hl, de
            ld      (v_input_start), hl
            ex      hl, de

            ; hl = указатель на номер первого кластера файла
            ld      hl, (v_foundDescrPtr)
            ld      bc, FILE_DESCRIPTOR.firstCluster
            add     hl, bc

            ; de = первый кластер файла
            ld      e, (hl)
            inc     hl
            ld      d, (hl)
            push    de

            ; de = первый сектор файла в области данных
            call    fatClusterToSector

            ; hl = указатель на длину найденного файла
            ld      hl, (v_foundDescrPtr)
            ld      bc, FILE_DESCRIPTOR.size + 1
            add     hl, bc

            ; c = количество кластеров файла. Просто читаем старший байт
            ; размера, т.к. кластер размером 256 байт.
            ; TODO: это не будет работать с другим размером кластера!
            ld      c, (hl)
            inc     c

            ; hl = адрес загрузки
            ld      hl, (v_input_start)

            ; Режим драйвера - чтение
            ld      b, 2
fileLoadLoop:
            ; Читаем кластер в память
            call    j_diskDriver    ; de = номер сектора, hl = адрес куда читать

            inc     h               ; следующий блок в памяти (TODO: это работает только для сектора 256 байт)

            ; Вычисляем следующий кластер в fat
            pop     de              ; de = номер текущего кластера
            call    fatReadCluster  ; de = номер следующего кластера
            push    de

            ; Переводим в номер сектора
            call    fatClusterToSector  ; de = следующий сектор файла в области данных

            dec     c               ; счётчик считанных кластеров
            jp nz,  fileLoadLoop    ; повтор цикла

            ; Восстанавливаем регистры
            pop     de
            pop     de
            pop     bc
            pop     hl

            jp      fileLoadExit

; ---------------------------------------------------------------------------
; Получить информацию о файле (дескриптор)
;
; На входе
;  hl - имя
;
; На выходе
;  bc, de, hl - сохраняются
;  v_foundDescrPtr - адрес копии дескриптора найденного файла
;  cf - ошибка
;----------------------------------------------------------------------------

fileLoadInfo:
            ; Ищем файл с именем в hl
            call    fileFind
            ret c 

; ---------------------------------------------------------------------------

fileLoadExit:
            ; Сохраняем регистры
            push    hl
            push    bc
            push    de

            ; de = указатель на первое поле после расширения
            ld      bc, FILE_DESCRIPTOR.attrib
            add     hl, bc
            ex      de, hl

            ; Информация о файле, исключая имя
            ld      hl, (v_foundDescrPtr)
            add     hl, bc

            ; Копируем дескриптор из hl в de, исключая имя и расширение
            ld      b, DIR_DESCR_SIZE - DIR_NAME_LENGTH - 3
fileLoadCDLoop:
            ld      a, (hl)
            ld      (de), a
            inc     hl
            inc     de
            dec     b
            jp nz,  fileLoadCDLoop

            ; Выход CF=0
            or      a
            jp      popDBH_ret

