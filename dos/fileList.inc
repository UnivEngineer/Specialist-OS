;+---------------------------------------------------------------------------
; MXOS
; Получить список файлов
;
; На входе 
;  hl - адрес буфера (769 байт)
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

            ; Максимум файлов
            ld      b, DIR_MAX_FILES

            ; Адрес первого файла
            ld      de, diskDirectory
            
fileList_loop:    ; Если первый символ не FFh, копируем описатель
            ld      a, (de)
            inc     a
            jp nz,  fileList_copy

            ; Вычисляем адрес следующего файла
            push    hl
            ld      hl, DIR_DESCR_SIZE
            add     hl, de
            ex      de, hl
            pop     hl

fileList_next:    ; Цикл
            dec     b
            jp nz,  fileList_loop

            ; В конце символ 0FFh
            ld      (hl), 0FFh
            jp      popDBh_ret

; ---------------------------------------------------------------------------

fileList_copy:    ; Копируем DIR_DESCR_SIZE байт из de в hl
            ld      c, DIR_DESCR_SIZE
fileList_copyl:     ld    a, (de)
            ld      (hl), a
            inc     hl
            inc     de
            dec     c
            jp nz,  fileList_copyl
            jp      fileList_next

