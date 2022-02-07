;+---------------------------------------------------------------------------
; MXOS
; Удаление файла
;
; На входе
;  hl - имя файла
;
; На выходе
;  cf - ошибка
;  bc, de, hl - сохраняются
;
; 2013-12-12 Дизассемблировано vinxru
;----------------------------------------------------------------------------

fileDelete: ; Найти файл с именем из hl
            call    fileFind
            ret     c 

            ; Удалить файл
            call    deleteFileInt

            ; Сохранить изменения на диск
            call    saveFatDir

            ; Результат
            or      a
            ret
            
; ---------------------------------------------------------------------------

deleteFileInt:
            ; Сохраняем регистры
            push    hl
            push    bc
            push    de

            ; Помечаем файл, как удаленный заменяя его первую букву его имени на 0FFh
            ld      hl, (v_foundedFile)
            ld      (hl), 0FFh          

            ; Получаем первый кластер файла в de
            ld      de, FILE_DESCRIPTOR.firstCluster
            add     hl, de
            ld      e, (hl)
            inc     hl
            ld      d, (hl)

deleteFileLoop:
            ld      h, d
            ld      l, e

            ; Обнуляем класте номер de и получаем номер
            ; следующиего кластера файла в de
            call    fatFreeCluster

            ; Если это был не последний кластер, продолжаем.
            ld      a, d
            cp      h
            jp nz,  deleteFileLoop
            ld      a, e
            cp      l
            jp nz,  deleteFileLoop

            ; Восстаналиваем регистры и выходим
            jp      popDBH_ret

