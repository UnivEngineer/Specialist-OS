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
; 2013-12-12 Дизассемблировано vinxru
; 2022-02-04 Переработано SpaceEngineer
;----------------------------------------------------------------------------

fileCreate: ; Сохраняем регистры
            push    hl
            push    bc
            push    de

            ; Если файл существует, то удаляем его
            call    fileFind
            call nc,deleteFileInt

            ; Сохраняем готовый дескриптор файла
            push    hl

            ; Ищем свободную запись в каталоге
            ld      hl, DIR_BUFFER      ; адрес каталога
            ld      de, DIR_DESCR_SIZE  ; размер записи в каталоге
            ld      b,  DIR_MAX_FILES   ; максимум файлов
fileCreate_0:
            ld      a, (hl)             ; читаем байт из каталога
            inc     a                   ; если байт = 0FFh, значит
            jp z,   fileCreate_1        ; свободная запись найдена
            add     hl, de              ; следующая запись
            dec     b                   ; счетчик макс. кол-ва файлов
            jp nz,  fileCreate_0

            ; Свободных записей нет

            ; Освобождаем стек
            pop     hl

            ; Установка a=0, ZF=1
            xor     a           

stc_popDBH_ret:
            ; Установка флага CF=1
            scf

popDBH_ret: ; Восстановление регистров и выход
            pop     de
            pop     bc
            pop     hl
            ret

; ---------------------------------------------------------------------------
; Свободная запись найдена

fileCreate_1:
            ; Сохраняем адрес дескриптора создаваемого файла в буфере каталога
            ld      (v_createdFile), hl

            ; Копируем в de
            ex      de, hl

            ; Адрес готового дескриптора файла в hl
            pop     hl

            ; Копируем готовый дескриптор файла в каталог (из hl в de)
            ld      c, DIR_DESCR_SIZE
fileCreate_2:
            ld      a, (hl)
            ld      (de), a
            inc     hl
            inc     de
            dec     c
            jp nz,  fileCreate_2

            call    fatFindClusterFirst ; de = первый свободный кластер
            jp c,   stc_popDBH_ret      ; если нет свободных кластеров - выходим с ошибкой

            ; Сохраняем его в переменную v_fileFirstCluster
            ex      hl, de
            ld      (v_fileFirstCluster), hl
            ex      hl, de

            ; Сохраняем в каталог номер первого кластера
            ; de = номер первого кластера
            ld      hl, (v_createdFile)
            ld      bc, FILE_DESCRIPTOR.firstCluster
            add     hl, bc  ; hl = адрес поля firstCluster (мл. байт) в дескрипторе файла
            ld      (hl), e
            inc     hl      ; hl = адрес поля firstCluster (ст. байт) в дескрипторе файла
            ld      (hl), d

		    ; Читаем адрес загрузки файла
            ; de = адрес загрузки файла
            ld      hl, (v_createdFile)
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
            ld      hl, (v_createdFile)
            ld      bc, FILE_DESCRIPTOR.size
            add     hl, bc  ; hl = адрес поля size (мл. байт) в дескрипторе файла
            ld      e, (hl)
            inc     hl      ; hl = адрес поля size (ст. байт) в дескрипторе файла
            ld      d, (hl)

            ; Сохраняем размер файла в переменную v_input_end
            ex      de, hl
            ld      (v_input_end), hl   ; v_input_end = размер файла

            ; Счётчик кластеров файла
            ld      c, h        ; c  = старший байт размера = количество кластеров файла - 1, т.к. кластер размером 256 байт.
            inc     c           ; c  = количество кластеров файла. TODO: это не будет работать с другим размером кластера!
            ld      b, c        ; сохраняем в b - пригодится позже

            ; Вычисляем первый кластер в fat
            ld      hl, (v_fileFirstCluster)    ; hl = номер первого кластера файла
            push    hl
            add     hl, hl
            ld      de, FAT_BUFFER   ; de = адрес таблицы fat в памяти
            add     hl, de          ; hl = указатель на первый кластер файла в таблице fat
            pop     de              ; de = номер первого кластера файла

            jp      writeFirstCluster

; ---------------------------------------------------------------------------
; Записываем кластеры файла в таблицу fat в памяти

allocClusters:
            call    fatFindClusterNext  ; de = следующий свободный кластер
            jp c,   stc_popDBH_ret      ; если больше нет свободных кластеров - выходим с ошибкой

writeFirstCluster:
            ; записываем номер кластера в fat и переходим на следущий
            ; hl = адрес ячейки с номером кластера
            ; de = номер кластера
            call    fatWriteCluster

            dec     c               ; счётчик записанных кластеров
            jp nz,  allocClusters   ; повтор цикла

            ; Конец, последний кластер файла указывает сам на себя
            ld      (hl), e
            inc     hl
            ld      (hl), d

; ---------------------------------------------------------------------------
; Сохраняем файл на диск

            ld      hl, (v_fileFirstCluster)
            ex      hl, de                      ; de = номер первого кластера файла
            ld      hl, (v_input_start)         ; hl = адрес загрузки файла
            ld      c, b                        ; c = количество кластеров файла
            ld      b, 1                        ; режим драйвера 1 - запись
writeFileLoop:
            call    j_diskDriver
            inc     h                           ; cледующий блок файла в памяти (TODO: это работает только для кластера 256 байт)

            ; Находим следующий кластер в fat
            call    fatReadCluster              ; de = номер следующего кластера

            dec     c                           ; счётчик записанных кластеров
            jp nz,  writeFileLoop               ; повтор цикла

; ---------------------------------------------------------------------------
; Расчет однобайтовой контрольной суммы

            ld      hl, (v_input_end)   ; hl = размер файла - 1 из v_input_end
            inc     hl                  ; hl = размер файла
            ex      de, hl              ; de = размер файла
            ld      hl, (v_input_start) ; hl = адрес загрузки файла из v_input_start
            ld      b, 0                ; тут будет накапливаться сумма
fileCreateCRCLoop:
            ld      a, d
            or      e
            jp z,   fileCreateExit      ; если счетчик досчитал до нуля, выходим
            ld  a,  (hl)                ; читаем байт файла
            add     a, b                ; накапливаем сумму
            ld      b, a
            inc     hl                  ; следующий байт
            dec     de                  ; уменьшаем счетчик
            jp      fileCreateCRCLoop

; ------------------------------------------------------------------------------------------------------------------------
; Сохранение и выход

fileCreateExit:
            ; Сохраняем контрольную сумму в дескриптор файла
            ld      a, b                ; a = посчитанная контрольная сумма
            ld      hl, (v_createdFile)
            ld      bc, FILE_DESCRIPTOR.checkSum
            add     hl, bc              ; hl = адрес поля checkSum в дескрипторе файла
            ld      (hl), a             ; сохраняем контрльную сумму

            call    saveFatDir          ; сохраняем fat и каталог на диск
            or      a                   ; выходим с флагом cf = 0
            jp      popDBH_ret

