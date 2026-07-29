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

fileLoad2:  ; загрузить файл с именем HL по адресу DE
            call    fileFind
            ret c

            push    hl
            push    bc
            push    de
            jp      fileLoadStep2

fileLoad:   ; загрузить файл с именем HL по адресу из дескриптора
            call    fileFind
            ret c

            push    hl
            push    bc
            push    de

            ld      hl, (v_foundDescrPtr)
            ld      de, FILE_DESCRIPTOR.loadAddress
            add     hl, de
            ld      e, (hl)
            inc     hl
            ld      d, (hl)

fileLoadStep2:
            ex      hl, de
            ld      (v_input_start), hl
            ex      hl, de

            ld      hl, (v_foundDescrPtr)
            ld      bc, FILE_DESCRIPTOR.firstCluster
            add     hl, bc
            ld      e, (hl)
            inc     hl
            ld      d, (hl)

            ld      hl, (v_foundDescrPtr)
            ld      bc, FILE_DESCRIPTOR.size + 1
            add     hl, bc
            ld      c, (hl)
            inc     c

            ld      hl, (v_input_start)
            ld      b, 2
            call    fileTransferClusters

            pop     de
            pop     bc
            pop     hl
            ret c
            jp      fileLoadExit

;----------------------------------------------------------------------------
; Передача последовательных кластеров текущего файла.
;
; Вход:  HL = буфер, DE = текущий кластер, B = 1 запись / 2 чтение,
;         C = число кластеров (0 означает 256).
; Выход: HL увеличен на C * 256, DE = следующий кластер,
;         BC сохранён, CF = ошибка драйвера.
;
; Текущая файловая система предполагает сектор 256 байт и один сектор
; в кластере. Диск выбирает вызывающая сторона.
;----------------------------------------------------------------------------

fileTransferClusters:
            push    bc
fileTransferClusters_loop:
            push    de
            call    fatClusterToSector
            or      a
            call    j_diskDriver
            pop     de
            jp c,   fileTransferClusters_error

            call    fatReadCluster
            inc     h
            dec     c
            jp nz,  fileTransferClusters_loop

            pop     bc
            or      a
            ret

fileTransferClusters_error:
            pop     bc
            scf
            ret

; ---------------------------------------------------------------------------
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
            ld      b, FILE_DESCR_SIZE - FILE_NAME_LENGTH - 3
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

