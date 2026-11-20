; Получить метаданные найденного файла, сохранив входное имя.
fatFileLoadInfo:
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
