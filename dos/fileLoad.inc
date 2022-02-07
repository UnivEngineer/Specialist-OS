;+---------------------------------------------------------------------------
; MXOS
; Загрузить файл
;
; На входе
;  hl - имя
;  de - адрес загрузки
;
; На выходе
;  bc, de, hl - сохраняются
;  v_foundedFile - найденный файл
;  cf - ошибка
;
; 2013-12-12 Дизассемблировано vinxru
; 2022-02-04 Переработано SpaceEngineer
;----------------------------------------------------------------------------

fileLoad2:  ; Ищем файл с именем в hl

            ; SpaceEngineer: функция fileFind2 пропускает обновление буфера fat и
            ; каталога текущего диска. Из-за этого fileLoad2 некорректно работает
            ; с файлами на ром-диске (A:, без поддержки записи - потому что изменение
            ; адреса загрузки программы сделано несколько костыльно, через патчинг
            ; этого адреса в структуре каталога). 
            ; Вызывающая программа могла бы сама вызвать loadFatDir перед вызовом
            ; fileLoad2, но, к сожалению, loadFatDir не числится в списке стандартных
            ; точек вызова.

            ;call    fileFind2
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
;  v_foundedFile - найденный файл
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
            ld      hl, (v_foundedFile)
            ld      de, FILE_DESCRIPTOR.loadAddress
            add     hl, de

            ; de = адрес загрузки
            ld      e, (hl)
            inc     hl
            ld      d, (hl)

fileLoadStep2:
            ; Сохраним адрес загрузки
            push    de

            ; hl = указатель на номер первого кластера файла
            ld      hl, (v_foundedFile)
            ld      bc, FILE_DESCRIPTOR.firstCluster
            add     hl, bc

            ; de = первый кластер файла
            ld      e, (hl)
            inc     hl
            ld      d, (hl)

            ; hl = указатель на длину найденного файла
            ld      hl, (v_foundedFile)
            ld      bc, FILE_DESCRIPTOR.size + 1
            add     hl, bc

            ; c = количество кластеров файла. Просто читаем старший байт
            ; размера, т.к. кластер размером 256 байт.
            ; TODO: это не будет работать с другим размером кластера!
            ld      c, (hl)
            inc     c

            ; hl = адрес загрузки
            pop     hl

fileLoadLoop:
            ; Читаем кластер в память
            ld      b, 2            ; b = 2 - режим чтения
            call    j_diskDriver    ; de = номер кластера, hl = адрес куда читать

            inc     h               ; следующий блок в памяти (TODO: это работает только для кластера 256 байт)

            ; Вычисляем следующий кластер в fat
            call    fatReadCluster  ; de = номер следующего кластера

            dec     c               ; счётчик записанных кластеров
            jp nz,  fileLoadLoop    ; повтор цикла

            ; Восстанавливаем регистры
            pop     de
            pop     bc
            pop     hl

            jp      fileLoadExit

; ---------------------------------------------------------------------------

fileLoadInfo:
            ; Ищем файл с именем в hl
            call    fileFind
            ret c 

fileLoadExit:
            ; Сохраняем регистры
            push    hl
            push    bc
            push    de

            ; de = указатель на первое поле после расширения
            ld      bc, FILE_DESCRIPTOR.ext + 3
            add     hl, bc
            ex      de, hl

            ; Информация о файле, исключая имя
            ld      hl, (v_foundedFile)
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

