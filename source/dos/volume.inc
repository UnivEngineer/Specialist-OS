;----------------------------------------------------------------------------
; Общие файловые точки DOS: выбор реализации по текущему тому.
; Вход/выход каждой обёртки соответствует прежнему файловому ABI.
; Диспетчер передаёт обработчику исходные AF/BC/DE/HL без изменения.
; Не реентерабелен: вызовы DOS и смена тома не выполняются из прерываний.
;----------------------------------------------------------------------------
fileList:
            push af
            ld a, VOL_LIST
            jp volumeDispatch
fileCreate:
            push af
            ld a, VOL_CREATE
            jp volumeDispatch
fileLoad:
            push af
            ld a, VOL_LOAD
            jp volumeDispatch
fileDelete:
            push af
            ld a, VOL_DELETE
            jp volumeDispatch
fileRename:
            push af
            ld a, VOL_RENAME
            jp volumeDispatch
fileLoadInfo:
            push af
            ld a, VOL_STAT
            jp volumeDispatch
fileGetSetAddr:
            push af
            ld a, VOL_LOAD_ADDRESS
            jp volumeDispatch
fileGetSetAttr:
            push af
            ld a, VOL_ATTRIBUTES
            jp volumeDispatch
fileLoad2:
            push af
            ld a, VOL_LOAD_AT
            jp volumeDispatch
getDriveFreeSpace:
            push af
            ld a, VOL_FREE_SPACE
            jp volumeDispatch
fileCreatePrepare:
            push af
            ld a, VOL_CREATE_PREPARE
            jp volumeDispatch
fileTransferClusters:
            push af
            ld a, VOL_TRANSFER_LEGACY
            jp volumeDispatch
flushVolume:
            push af
            ld a, VOL_FLUSH
            jp volumeDispatch

; HL = [диск:]имя, DE = буфер. Префикс диска принадлежит DOS,
; оставшуюся часть имени разбирает файловый драйвер выбранного тома.
fileNamePrepare:
            push hl
            inc hl
            ld a, (hl)
            cp ':'
            pop hl
            jp nz, volumeNameReady
            ld a, (hl)
            sub 'A'
            call fileSelectDrive
            inc hl
            inc hl
volumeNameReady:
            push af
            ld a, VOL_PREPARE_NAME
            jp volumeDispatch

; A = смещение обработчика, исходный AF уже лежит на стеке.
; 22 байта, 139 тактов до входа обработчика (плюс 28 у обёртки).
; Вместе с обёрткой шесть записей и шесть чтений байтов стека,
; четыре чтения таблицы/указателя,
; две записи адреса перехода; экран, диски и порты не затрагиваются.
volumeDispatch:
            push hl
            push de
            ld e, a
            ld d, 0
            ld hl, (v_volumeTable)
            add hl, de
            ld e, (hl)
            inc hl
            ld d, (hl)
            ex de, hl
            ld (volumeJump+1), hl
            pop de
            pop hl
            pop af
volumeJump:
            jp 0

;----------------------------------------------------------------------------
; Регистрация: A = диск 0..7, HL = таблица "FS",1,15 + DW обработчиков,
; DE = адрес контекста, управляемого вызывающей программой (можно 0).
; BC/DE/HL сохраняются, CF=1 при плохом индексе/заголовке; A разрушается.
; Для выбранного диска выполняется перемонтирование, иначе — при выборе.
; Перед заменой активного модуля вызывающая сторона обязана сделать flush
; и закрыть свои операции. Нулевой обработчик в таблице недопустим.
;----------------------------------------------------------------------------
installVolume:
            cp 8
            jp nc, volumeArgumentError
            push bc
            push de
            push hl
            push af
            ld a, (hl)
            cp 'F'
            jp nz, volumeBadHeader
            inc hl
            ld a, (hl)
            cp 'S'
            jp nz, volumeBadHeader
            inc hl
            ld a, (hl)
            cp VOLUME_DRIVER_VERSION
            jp nz, volumeBadHeader
            inc hl
            ld a, (hl)
            cp VOLUME_DRIVER_COUNT
            jp nz, volumeBadHeader
            ; Все обработчики обязательны: отказ должен быть процедурой с CF.
            ld b, VOLUME_DRIVER_COUNT
volumeValidateLoop:
            inc hl
            ld a, (hl)
            inc hl
            or (hl)
            jp z, volumeBadHeader
            dec b
            jp nz, volumeValidateLoop

            pop af
            ld c, a
            pop hl
            push hl
            ex de, hl
            push hl                 ; контекст
            push de                 ; таблица
            ld hl, v_volumes
            call volumeSlot
            pop de
            ld (hl), e
            inc hl
            ld (hl), d
            ld hl, v_volumeContexts
            call volumeSlot
            pop de
            ld (hl), e
            inc hl
            ld (hl), d
            ld a, (v_drive)
            cp c
            pop hl
            pop de
            pop bc
            jp z, mountVolume
            or a
            ret

volumeBadHeader:
            pop af
            pop hl
            pop de
            pop bc
volumeArgumentError:
            ld a, 1
            scf
            ret

; Внутреннее: C = диск, HL = начало массива слов; HL = элемент.
; A/DE/флаги разрушаются, BC сохраняется.
volumeSlot:
            ld a, c
            add a, a
            ld e, a
            ld d, 0
            add hl, de
            ret

; A = диск 0..7. Выход HL = таблица, DE = контекст; BC сохраняется.
; CF=1 при неправильном индексе. Вызов не меняет выбранный диск.
getVolume:
            cp 8
            jp nc, volumeArgumentError
            push bc
            ld c, a
            ld hl, v_volumes
            call volumeSlot
            ld e, (hl)
            inc hl
            ld d, (hl)
            push de
            ld hl, v_volumeContexts
            call volumeSlot
            ld e, (hl)
            inc hl
            ld d, (hl)
            pop hl
            pop bc
            or a
            ret

; Выбранный том: установить таблицу/контекст, сбросить старый кэш,
; затем вызвать mount. BC/DE/HL сохраняются, AF = результат обработчика.
; После подмены ядра все модули/контексты регистрируются заново.
mountVolume:
            push hl
            push de
            push bc
            ld a, (v_drive)
            call getVolume
            ex de, hl
            ld (v_volumeContext), hl
            ex de, hl
            ld de, VOLUME_DRIVER_HEADER
            add hl, de
            ld (v_volumeTable), hl
            call resetSectorCache
            call volumeMountDispatch
            pop bc
            pop de
            pop hl
            ret
volumeMountDispatch:
            push af
            ld a, VOL_MOUNT
            jp volumeDispatch

; Существующий формат MXOS. Байты на носителе и файловые алгоритмы
; в этом этапе не меняются; новые драйверы не вызывают эти внутренние метки.
fatVolumeMount:
            call fatReadBootSector
            ld a, (v_diskInfo.isValid)
            cp DISK_VALID
            ret z
            scf
            ret
fatVolumeFlush:
            xor a
            ld (bios_vars.diskWriteErr), a
            call saveSectorCache
            ld a, (bios_vars.diskWriteErr)
            or a
            ret z
            scf
            ret

fatVolumeDriver:
            DB "FS", VOLUME_DRIVER_VERSION, VOLUME_DRIVER_COUNT
            DW fatVolumeMount, fatFileList, fatFileCreate, fatFileLoad
            DW fatFileDelete, fatFileRename, fatFileLoadInfo
            DW fatFileGetSetAddr, fatFileGetSetAttr, fatFileLoad2
            DW fatGetDriveFreeSpace, fatFileNamePrepare
            DW fatFileCreatePrepare, fatFileTransferClusters, fatVolumeFlush
            ASSERT $-fatVolumeDriver == VOLUME_DRIVER_HEADER + VOLUME_DRIVER_COUNT*2
