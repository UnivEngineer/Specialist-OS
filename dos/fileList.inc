;+---------------------------------------------------------------------------
; MXOS
; Получить список файлов каталога.
; Функция копирует в буфер непустые дескрипторы файлов.
;
; На входе 
;  hl - адрес буфера
;  b  - максимальное количество дескрипторов файлов для вывода
;
; На выходе
;  bc, de, hl - сохраняются
;
; 2013-12-12 Дизассемблировано vinxru
;----------------------------------------------------------------------------

fileList:   ; Загрузка FAT и каталога
            call    loadFatDir

            ; Сохранение регистров
            push    hl
            push    bc
            push    de

            ; Адрес первого файла
            ld      de, DIR_BUFFER

            ; Максимум файлов уже в b
            ; Но надо ограничить разером каталога устройства
            ld      a, DIR_MAX_FILES
            cp      b
            jp nc,  fileList_loop
            ld      b, a    ; b = max(b, DIR_MAX_FILES)

fileList_loop:
            ; Если первый символ не FFh, копируем дескриптор
            ld      a, (de)
            inc     a
            jp nz,  fileList_copy

            ; Вычисляем адрес следующего файла
            push    hl
            ld      hl, DIR_DESCR_SIZE
            add     hl, de
            ex      de, hl
            pop     hl

fileList_next:
            ; Цикл
            dec     b
            jp nz,  fileList_loop

            ; В конце символ 0FFh
            ld      (hl), 0FFh
            jp      popDBH_ret

; ---------------------------------------------------------------------------

fileList_copy:
            ; Копируем DIR_DESCR_SIZE байт из de в hl
            ld      c, DIR_DESCR_SIZE
fileList_copyl:     ld    a, (de)
            ld      (hl), a
            inc     hl
            inc     de
            dec     c
            jp nz,  fileList_copyl
            jp      fileList_next

