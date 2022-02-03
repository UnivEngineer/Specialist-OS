;+---------------------------------------------------------------------------
; MXOS
; Найти файл
;
; На входе
;  hl - имя
;
; На выходе
;  bc, de, hl - сохраняются
;  cf - ошибка
;  v_foundedFile - найденный файл
;
; 2013-12-12 Дизассемблировано vinxru
;----------------------------------------------------------------------------

fileFind:   ; Загрузка FAT и корневого каталога в память
            call    loadFatDir

fileFind2:  ; Сохраняем регистры
            push    bc
            push    de

            ; Начало каталога
            ld      de, diskDirectory

            ; Кол-во файлов в каталоге
            ld      b, DIR_MAX_FILES

fileFind_loop:    ; Сохраняем адрес начала имени в каталоге и образец
            push    hl
            push    de

            ; Если первый символ имени 0FFh, переходим к следующем файлу
            ld      a, (de)
            inc     a
            jp z,   fileFind_next

            ; Длина имени + расширения
            ld      c, DIR_NAME_LENGTH + 3

fileFind_name:    ; Если символы не равны, переходим к следующему файлу
            ld      a, (de)
            cp      (hl)
            jp nz,  fileFind_next
            inc     hl
            inc     de

            ; Цикл
            dec     c
            jp nz,  fileFind_name

            ; Сохраняем адрес найденного файла
            pop     hl
            ld      (v_foundedFile), hl

            ; Восстаналиваем регистры и выходим с CF=0
            pop     hl
            or      a     ; х.з.
            pop     de
            pop     bc
            ret

; ---------------------------------------------------------------------------

fileFind_next:    ; Вычисляем адрес следующего файла
            pop   de
            ld    hl, DIR_DESCR_SIZE
            add   hl, de
            ex    de, hl

            ; Восстанавливаем образец
            pop   hl

            ; Еще остались файлы?
            dec   b
            jp nz,  fileFind_loop

            ; Восстаналиваем регистры и выходим с CF=1 и a=3
            pop   de
            pop   bc
            ld    a, 3
            scf
            ret

