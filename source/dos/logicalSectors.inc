; Логический сектор состоит из одного или нескольких физических блоков.
; B=1 запись / 2 чтение, DE=LBA тома, HL=буфер полного сектора.
; Внутренний logicalSectorPrepared также принимает B=5 (запись с проверкой).
; BC/DE/HL сохранены, AF разрушен, CF=ошибка. Нереентерабельно, DI.
; Секторы меньше блока драйвера отвергаются: скрытой RMW-записи нет.

;----------------------------------------------------------------------------
; MXOS
;
; Передать один логический сектор через физический драйвер текущего диска.
;
; вход:
;   b = 1 для записи или 2 для чтения, de = LBA тома, hl = буфер размером
;   v_diskInfo.sectorSize. Геометрия задана при монтировании.
;
; выход:
;   cf = 0 при успехе; при недопустимой операции или LBA cf = 1 и a = DOS_ERR_ARGUMENT.
;   При отказе драйвера cf = 1. bc, de и hl сохранены; передача могла остановиться
;   после части физических блоков.
;----------------------------------------------------------------------------

logicalSectorIO:
            ld a,b
            dec a
            cp 2
            jp nc,blockArgument

;----------------------------------------------------------------------------
; MXOS
;
; Проверить номер логического сектора и передать его драйверу.
;
; вход:
;   b уже проверен (1 или 2), de = LBA, hl = буфер полного сектора. v_logicalCapacity
;   задаёт верхнюю границу исключительно.
;
; выход:
;   Для LBA вне тома возвращаются cf = 1 и a = DOS_ERR_ARGUMENT. Для допустимого LBA
;   возвращается результат logicalSectorPrepared. bc, de и hl сохранены.
;----------------------------------------------------------------------------

logicalSectorIOChecked:
            ; Внутренние вызывающие задают только B=1/2.
            push hl
            ld hl,(v_logicalCapacity)
            call cmp_hl_de
            pop hl
            jp c,blockArgument
            jp z,blockArgument

;----------------------------------------------------------------------------
; MXOS
;
; Передать ранее проверенный сектор без повторной проверки адреса.
;
; вход:
;   b = 1 (запись), 2 (чтение) или 5 (проверяемая запись), de = допустимый LBA, hl =
;   проверенный буфер. v_logicalShift задаёт степень двойки отношения размеров сектора
;   и блока.
;
; выход:
;   cf = 0 при успехе. При отказе физического блока cf = 1, последующие блоки не
;   переданы. bc, de и hl сохранены, af используется при маршрутизации.
;----------------------------------------------------------------------------

logicalSectorPrepared:
            ; Внутренний поток уже проверил кластер и весь буфер.
            ; BPB гарантирует, что его сектора лежат внутри тома.
            ld a,(v_logicalShift)
            or a
            jp z,logicalPhysicalIO
            push hl
            push de
            push bc
            ld c,1
            ex de,hl
logicalScale:
            ; DE<LBA capacity уже проверено: масштабирование не переполняется.
            add hl,hl
            push af
            ld a,c
            add a,a
            ld c,a
            pop af
            dec a
            jp nz,logicalScale
            ex de,hl
logicalBlocks:
            call logicalPhysicalIO
            jp c,logicalDone
            inc de
            push de
            ex de,hl
            ld hl,(v_diskInfo.blockSize)
            add hl,de
            pop de
            dec c
            jp nz,logicalBlocks
            or a
logicalDone:
            pop bc
            pop de
            pop hl
            ret

logicalPhysicalIO:
    IFDEF BANKED_DOS_BUILD
            ; Диск выбран до начала операции, B=1/2 задан вызывающим.
            ; Не повторяем выбор устройства и разбор номера функции.
            ld a,b
            cp 5
            jp z,dosRawIO
            jp j_diskDriver
    ELSE
            ld a,(v_drive)
            jp bios_blockIO
    ENDIF

;----------------------------------------------------------------------------
; MXOS
;
; Перевести двоичный порядок размера сектора в число байт.
;
; вход:
;   a = 7, 8 или 9.
;
; выход:
;   hl = 128, 256 или 512 соответственно. bc и de сохранены, af рабочий.
;----------------------------------------------------------------------------

sectorSizeFromShift:
            ld hl,0080h
            sub 7
            ret z
sectorSizeLoop:
            add hl,hl
            dec a
            jp nz,sectorSizeLoop
            ret

; Геометрия входит в tag строки; старые строки этого диска при её смене
; удаляются. Общая таблица и данные других дисков остаются на месте.
configureSectorCache:
            ld a,(v_drive)
            ld (v_cacheDrive),a
            ld e,a
            ld d,0
            ld hl,v_cacheGeometries
            add hl,de
            ld a,(v_diskInfo.sectorShift)
            cp (hl)
            jp z,cacheGeometryReady
            ld (hl),a
            ld a,e
            call invalidateDriveCache
cacheGeometryReady:
            ld a,(v_diskInfo.sectorShift)
            ld (v_cacheShift),a
            sub 7
            ld b,a
            rlca
            rlca
            rlca
            or e
            ld (v_cacheTag),a
            ld a,b
            ld b,1
            or a
            jp z,cacheSpanReady
            ld b,2
            dec a
            jp z,cacheSpanReady
            ld b,4
cacheSpanReady:
            ld a,b
            ld (v_cacheSpan),a
            dec a
            cpl
            ld (v_cacheMask),a
            ld l,b
            ld h,0
            add hl,hl
            add hl,hl
            add hl,hl
            ld (streamCacheProbeStep+1),hl
            jp cacheUpdateProbeCount

; Достаточно проверять только индексы, выровненные по размеру искомого
; сектора. Например, 512-байтная голова не бывает в грануле 1, 2 или 3.
cacheUpdateProbeCount:
            ld a,(v_cacheUsed)
            ld hl,v_cacheSpan
            add a,(hl)
            dec a
            ld b,a
            ld a,(v_cacheShift)
            sub 7
cacheProbeDivide:
            or a
            jp z,cacheProbeCountReady
            push af
            ld a,b
            or a
            rra
            ld b,a
            pop af
            dec a
            jp cacheProbeDivide
cacheProbeCountReady:
            ld a,b
            ld (v_cacheProbeCount),a
            ret

fatCachedWrite:
            call logicalSectorIOChecked
            ret c
            ld a,(v_diskInfo.fatCount)
            cp 2
            jp z,fatMirrorWrite
            or a
            ret
fatMirrorWrite:
            push hl
            push de
            ex de,hl
            push hl
            ld hl,(v_diskInfo.resSectors)
            ex de,hl
            pop hl
            call sub_hl_de
            jp c,fatMirrorDone
            push hl
            ld hl,(v_diskInfo.fatSectors)
            ex de,hl
            pop hl
            call cmp_hl_de
            jp nc,fatMirrorDone
            pop hl                ; исходный сектор
            push hl
            add hl,de
            ex de,hl
            pop hl                ; исходный сектор
            ex (sp),hl            ; HL=буфер, исходный сектор сохранён
            call logicalSectorIOChecked
            pop de
            ret
fatMirrorDone:
            pop de
            pop hl
            or a
            ret

blockArgument:
            ld a,DOS_ERR_ARGUMENT
            jp dosSetError

; HL / 2^A, A=6..9. HL=частное, DE=остаток; BC сохранены, AF разрушен.
; Деление 256-байтного сектора сводится к перестановке байтов (42 такта
; без CALL); младшие 6/7 бит FAT и 9-битные секторы обрабатываются отдельно.

;----------------------------------------------------------------------------
; MXOS
;
; Разделить смещение на номер сектора и остаток внутри него.
;
; вход:
;   hl = беззнаковое смещение, a = порядок размера от 6 до 9.
;
; выход:
;   hl = частное от деления на 2^a, de = остаток. bc сохранён, af меняется.
;----------------------------------------------------------------------------

splitSectorOffset:
            ld e,l
            ld d,0
            ld l,h
            ld h,0
            cp 8
            ret z
            jp c,splitSmall
            ld a,l
            and 1
            ld d,a
            ld a,l
            or a
            rra
            ld l,a
            ret
splitSmall:
            cp 7
            jp z,splitSeven
            add hl,hl
            add hl,hl
            ld a,e
            rlca
            rlca
            and 3
            add a,l
            ld l,a
            ld a,e
            and 03Fh
            ld e,a
            ret
splitSeven:
            add hl,hl
            ld a,e
            rlca
            and 1
            add a,l
            ld l,a
            ld a,e
            and 07Fh
            ld e,a
            ret
