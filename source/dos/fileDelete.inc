;+---------------------------------------------------------------------------
; MXOS
; Удаление файла
;
; вход:
;  hl - имя файла
;
; выход:
;  cf = 0 при успехе. При отказе cf = 1, a = код DOS.
;  hl и de сохраняются, b может измениться при сбросе кэша.
;
; 2013-12-12 Дизассемблировано vinxru
; 2022-02-11 Переработано SpaceEngineer
;----------------------------------------------------------------------------

fatFileDelete: ; Найти файл с именем из hl
            call    fileFind
            ret     c 

            ; Флаги System и Read-only запрещают удаление.
            call    fileCheckDeleteAllowed
            ret c

            ; Удалить файл
            call    deleteFileInt
            ret c

            ; Сохранить изменения на диск и дождаться физической записи.
            jp      fatVolumeFlush
            
; ---------------------------------------------------------------------------

; Проверить атрибуты найденной записи. CF=1: удаление запрещено.
; AF разрушается; BC, DE и HL сохраняются.

;----------------------------------------------------------------------------
; MXOS
;
; Проверить запрет на удаление найденной записи.
;
; вход:
;   v_tmpFileDescr содержит дескриптор объекта.
;
; выход:
;   При отсутствии атрибутов Read-only и System a = 0, cf = 0. Иначе a и v_dosError =
;   DOS_ERR_ACCESS, cf = 1. bc, de и hl сохранены.
;----------------------------------------------------------------------------

fileCheckDeleteAllowed:
            ld      a, (v_tmpFileDescr + FILE_DESCRIPTOR.attrib)
            and     005h
            ret z
            ld a,DOS_ERR_ACCESS
            jp dosSetError

;----------------------------------------------------------------------------
; MXOS
;
; Удалить найденную запись и освободить её кластеры.
;
; вход:
;   v_tmpFileDescr и курсор каталога задают запись на активном v_drive.
;   Проверку атрибутов и сброс изменений на диск выполняет вызывающий код.
;
; выход:
;   cf = 0 при успехе. При активном BAT или защите тома в банковой DOS запись
;   и цепочка сохранены, cf = 1, a и v_dosError содержат соответствующий код DOS.
;   Другие ошибки возвращаются от FAT и кэша.
;   bc, de и hl сохранены.
;----------------------------------------------------------------------------
deleteFileInt:
            push    hl
            push    bc
            push    de
    IFDEF BANKED_DOS_BUILD
            ; Режим тома известен до изменения записи каталога и кэша FAT.
            call registryInfo
            ld a,c
            and DISK_CAP_WRITE
            ld a,DOS_ERR_WRITE_PROTECTED
            jp z,deleteFileDenied
    ENDIF
            ; Сюда также входят RMDIR, рекурсивное удаление и замена файла.
            ld a,(v_batDepth)
            ld hl,(v_batFrame)
            call batFindActive
            jp nc,deleteFileAllowed
            ld a,DOS_ERR_ACCESS
deleteFileDenied:
            call dosSetError
            jp popDBH_ret
deleteFileAllowed:
            xor a
            ld (fatWriteError),a

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
            ld      (fatWriteError), a
            ret

;----------------------------------------------------------------------------
; MXOS
;
; Сбросить изменённые строки кэша и дождаться завершения физической записи.
;
; вход:
;   Выбраны том и его физический драйвер. Общие структуры кэша доступны до возврата.
;
; выход:
;   a = 0, cf = 0 при успехе. Ошибка записи кэша или физического барьера оставляет
;   fatWriteError ненулевым и возвращает cf = 1.
;----------------------------------------------------------------------------

fatVolumeFlush:
    xor a
    ld (fatWriteError),a
    call saveSectorCache
; После записи кэша физический барьер сохраняет все предыдущие ошибки.

;----------------------------------------------------------------------------
; MXOS
;
; Завершить серию записей физическим барьером, учитывая предыдущие ошибки.
;
; вход:
;   fatWriteError аккумулирует ошибки текущей операции. Выбран драйвер записываемого
;   тома.
;
; выход:
;   a = итоговое значение fatWriteError. При a = 0 выставлен z и сброшен cf, при
;   ненулевом a установлен cf.
;----------------------------------------------------------------------------

fatFinishWrite:
            call    fatFlushPhysical
            ld      a, (fatWriteError)
            or      a
            ret z
            scf
            ret

