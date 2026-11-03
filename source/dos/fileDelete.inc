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

fatFileDelete: ; Найти файл с именем из hl
            xor     a
            ld      (bios_vars.diskWriteErr), a
            call    fileFind
            ret     c 

            ; Флаги System и Read-only запрещают удаление.
            call    fileCheckDeleteAllowed
            ret c

            ; Удалить файл
            call    deleteFileInt

            ; Сохранить изменения на диск
            call    saveSectorCache

            ; Результат
            jp      fatFinishWrite
            
; ---------------------------------------------------------------------------

; Проверить атрибуты найденной записи. CF=1: удаление запрещено.
; AF разрушается; BC, DE и HL сохраняются.
fileCheckDeleteAllowed:
            ld      a, (v_tmpFileDescr + FILE_DESCRIPTOR.attrib)
            and     005h
            ret z
            scf
            ret

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

            ld a,d
            or e
            jp z,popDBH_ret
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

; Физический барьер. Без шага чипов драйвер синхронный.
; Выход CF=ошибка; AF разрушается, BC/DE/HL сохраняются.
fatFlushPhysical:
            push    hl
            ld      hl, (v_diskInfo.allocationStep)
            ld      a, h
            or      l
            pop     hl
            ret z
            push    bc
            ld      b, 4
            call    j_diskDriver
            pop     bc
            ret nc
            ld      a, 1
            ld      (bios_vars.diskWriteErr), a
            ret

; Завершение изменения файла после записи кэша и физического барьера.
; Вход: diskWriteErr от записей, в том числе при вытеснении.
; Выход: CF=ошибка; AF разрушается, BC/DE/HL сохраняются.
; При отказе сбрасываем кэш. Уже записанные секторы это не откатывает.
fatFinishWrite:
            call    fatFlushPhysical
            ld      a, (bios_vars.diskWriteErr)
            or      a
            ret z
            push    bc
            push    de
            push    hl
            call    resetSectorCache
            pop     hl
            pop     de
            pop     bc
            scf
            ret

