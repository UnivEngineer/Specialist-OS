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
            ld      (bios_upper.diskWriteErr), a
            call    fileFind
            ret     c 

            ; Флаги System и Read-only запрещают удаление.
            call    fileCheckDeleteAllowed
            ret c

            ; Удалить файл
            call    deleteFileInt
            ret c

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
            ld a,DOS_ERR_ACCESS
            jp dosSetError

deleteFileInt:
            ; Сохраняем регистры
            push    hl
            push    bc
            push    de

            ; Сначала сохраняем первый кластер: кэш может переместить запись.
            ld hl,(v_tmpFileDescr + FILE_DESCRIPTOR.firstCluster)
            ex de,hl
            push de
            call markDirSectorAsModified
            pop de
            jp c,popDBH_ret
            ld (hl),0E5h
            ld a,d
            or e
            jp z,popDBH_ret
deleteFileLoop:
            call fatCheckCluster
            jp c,popDBH_ret
            call fatFreeCluster
            jp c,popDBH_ret
            ld a,d
            cp 0FFh
            jp nz,deleteFileLoop
            ld a,e
            cp 0F8h
            jp c,deleteFileLoop
            or a
            jp popDBH_ret

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
            ld      (bios_upper.diskWriteErr), a
            ret

; Завершение изменения файла после записи кэша и физического барьера.
; Вход: diskWriteErr от записей, в том числе при вытеснении.
; Выход: CF=ошибка; AF разрушается, BC/DE/HL сохраняются.
; При отказе сбрасываем кэш. Уже записанные секторы это не откатывает.
fatFinishWrite:
            call    fatFlushPhysical
            ld      a, (bios_upper.diskWriteErr)
            or      a
            ret z
            scf
            ret

