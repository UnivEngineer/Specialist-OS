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
; 2022-02-11 Переработано SpaceEngineer
;----------------------------------------------------------------------------

fileDelete: ; Найти файл с именем из hl
            call    fileFind
            ret     c 

            ; Удалить файл
            call    deleteFileInt

            ; Сохранить изменения на диск
            call    saveSectorCache

            ; Результат
            or      a
            ret
            
; ---------------------------------------------------------------------------

deleteFileInt:
            ; Сохраняем регистры
            push    hl
            push    bc
            push    de

            ; Помечаем файл в кэше, как удаленный, заменяя первую букву его имени на 0FFh
            ld      hl, (v_cachedDescrPtr)
            ld      (hl), 0FFh
            push    hl

            ; Ставим сектору каталога флаг isModified
            call    markDirSectorAsModified

            ; Получаем первый кластер файла в de
            pop     hl
            ld      de, FILE_DESCRIPTOR.firstCluster
            add     hl, de
            ld      e, (hl)
            inc     hl
            ld      d, (hl)

deleteFileLoop:
            ld      h, d
            ld      l, e

            ; Обнуляем кластер номер de и получаем номер
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

