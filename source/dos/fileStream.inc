; Публичный ABI C8D2h. A=1 запись / 2 чтение, DE=FILE_STREAM_CURSOR,
; HL=буфер, BC=число байтов (0 — пустая операция).
; Выход: HL=первый непереданный байт, BC=передано, DE сохранён;
; курсор обновлён и при частичном отказе. A=код DOS, CF=ошибка.
; Буфер и курсор не пересекаются; память принадлежит вызывающей программе.
; Работает с активным FAT-томом и уже выделенной цепочкой; не расширяет файл,
; не меняет его размер и не ограничивает чтение размером из каталога.
; Запись отложенная: окончание операции требует bios_flushVolume.
; Нереентерабельно, как остальные файловые службы DOS.
fileStream:
            cp 1
            jp c,fileStreamArgument
            cp 3
            jp nc,fileStreamArgument
            push af
            xor a
            ld (v_dosError),a
            ld a,b
            or c
            jp z,fileStreamEmpty
            call fatCheckMounted
            jp c,fileStreamRangeError
            call streamBufferRange
            jp c,fileStreamRangeError
            push hl
            push bc
            ex de,hl
            ld bc,FILE_STREAM_CURSOR
            call streamBufferRange
            ex de,hl
            pop bc
            pop hl
            jp c,fileStreamRangeError
            pop af
            push de
            push bc
            push hl
            push af
            ex de,hl
            ld e,(hl)
            inc hl
            ld d,(hl)
            inc hl
            ld a,(hl)
            inc hl
            ld h,(hl)
            ld l,a
            ld (v_streamOffset),hl
            push de
            ex de,hl
            ld hl,(v_diskInfo.clusterBytes)
            ex de,hl
            call cmp_hl_de
            pop de
            jp c,fileStreamOffsetReady
            jp nz,fileStreamBadOffset
fileStreamOffsetReady:
            pop af
            pop hl
            call fatStream
            jp fileStreamResult
fileStreamBadOffset:
            pop af
            pop hl
            ld (v_streamPtr),hl
            ex de,hl
            ld (v_streamCluster),hl
            ld h,b
            ld l,c
            ld (v_streamRemaining),hl
            ld a,DOS_ERR_ARGUMENT
            call dosSetError
fileStreamResult:
            call c,dosIoError
            pop bc
            pop de
            push de
            ld hl,(v_streamCluster)
            ex de,hl
            ld (hl),e
            inc hl
            ld (hl),d
            inc hl
            ex de,hl
            ld hl,(v_streamOffset)
            ex de,hl
            ld (hl),e
            inc hl
            ld (hl),d
            ld hl,(v_streamRemaining)
            ex de,hl
            ld h,b
            ld l,c
            call sub_hl_de
            ld b,h
            ld c,l
            ld hl,(v_streamPtr)
            pop de
            jp dosGetError
fileStreamRangeError:
            pop af
            ld bc,0
            jp dosGetError
fileStreamEmpty:
            pop af
            xor a
            ret
fileStreamArgument:
            ld bc,0
            ld a,DOS_ERR_ARGUMENT
            jp dosSetError

; Общий внутренний поток: A=1/2 запись/чтение, HL=буфер, BC=байты,
; DE=кластер, v_streamOffset=смещение в кластере. 0 байт означает 0.
; Выход HL=конец буфера, DE=следующий кластер, v_streamOffset обновлён.
; BC/AF разрушены, CF=ошибка. Нереентерабельно, DI; буфер принадлежит программе.
fatStream:
    IFDEF BANKED_DOS_BUILD
            push af
            ld a,(kdFarActive)
            or a
            jp nz,kdFarStream
            pop af
fatStreamLocal:
    ENDIF
            ld (v_streamMode),a
            ld (v_streamPtr),hl
            ex de,hl
            ld (v_streamCluster),hl
            ld h,b
            ld l,c
            ld (v_streamRemaining),hl
streamLoop:
            ld hl,(v_diskInfo.clusterBytes)
            ex de,hl
            ld hl,(v_streamOffset)
            call cmp_hl_de
            jp z,streamNextCluster
            ld hl,(v_streamRemaining)
            ld a,h
            or l
            jp z,streamReturn
            ld hl,(v_streamCluster)
            ex de,hl
            call fatCheckCluster
            jp c,streamReturn
            ld hl,(v_streamOffset)
            ld a,(v_diskInfo.sectorShift)
            call splitSectorOffset
            push hl
            ex de,hl
            ld (v_streamWithin),hl
            ld hl,(v_streamCluster)
            ex de,hl
            call fatClusterToSector
            pop hl
            add hl,de
            ld (v_streamSector),hl
streamSectorLoop:
            ld hl,(v_streamWithin)
            ex de,hl
            ld hl,(v_diskInfo.sectorSize)
            call sub_hl_de
            ex de,hl
            ld hl,(v_streamRemaining)
            call cmp_hl_de
            jp c,streamChunkReady
            ex de,hl
streamChunkReady:
            ld (v_streamChunk),hl
            ld hl,(v_streamSector)
            ex de,hl
            ld hl,(v_streamWithin)
            ld a,h
            or l
            jp nz,streamCached
            push de
            ld hl,(v_streamChunk)
            ex de,hl
            ld hl,(v_diskInfo.sectorSize)
            call cmp_hl_de
            pop de
            jp nz,streamCached
            call streamCacheContains
            jp nc,streamCachedHit
            ; Полный сектор без копии в кэше передаётся прямо из/в буфер.
            ld hl,(v_streamPtr)
            ld a,(v_streamMode)
            ld b,a
    IFDEF BANKED_DOS_BUILD
            call ksSector
    ELSE
            call logicalSectorPrepared
    ENDIF
            jp c,streamReturn
            jp streamCopyDone
streamCached:
            ld a,(v_streamMode)
            and 1                 ; запись -> dirty=1, чтение -> 0
streamGetCache:
            call getSectorAddressInCache
            jp c,streamReturn
streamCachedAddress:
            ex de,hl
            ld hl,(v_streamWithin)
            add hl,de
            push hl               ; источник/назначение в кэше
            ld hl,(v_streamPtr)
            ex de,hl
            pop hl
            ld a,(v_streamMode)
            dec a
            jp nz,streamCopy
            ex de,hl
streamCopy:
            push hl
            ld hl,(v_streamChunk)
            ld b,h
            ld c,l
            pop hl
    IFDEF BANKED_DOS_BUILD
            call ksCacheCopy
    ELSE
            call moveBytesForward
    ENDIF
streamCopyDone:
            ld hl,(v_streamChunk)
            ex de,hl
            ld hl,(v_streamPtr)
            add hl,de
            ld (v_streamPtr),hl
            ld hl,(v_streamRemaining)
            call sub_hl_de
            ld (v_streamRemaining),hl
            ld hl,(v_streamOffset)
            add hl,de
            ld (v_streamOffset),hl
            ex de,hl
            ld hl,(v_diskInfo.clusterBytes)
            ex de,hl
            call cmp_hl_de
            jp nc,streamNextCluster
            ld hl,(v_streamRemaining)
            ld a,h
            or l
            jp z,streamReturn
            ; Внутри кластера следующий фрагмент начинается с нового сектора.
            ld hl,(v_streamSector)
            inc hl
            ld (v_streamSector),hl
            ld hl,0
            ld (v_streamWithin),hl
            jp streamSectorLoop
streamNextCluster:
            ld hl,(v_streamCluster)
            ex de,hl
            call fatReadCluster
            jp c,streamReturn
            ex de,hl
            ld (v_streamCluster),hl
            ld hl,0
            ld (v_streamOffset),hl
            jp streamLoop
streamReturn:
            push af
            ld hl,(v_streamCluster)
            ex de,hl
            ld hl,(v_streamPtr)
            pop af
            ret

; HL=дескриптор. DE=ceil(32-битная длина / clusterBytes), CF=переполнение.
; HL/BC сохранены; AF разрушен. Размер кластера — степень двойки.
fatSizeToClusters:
            push hl
            push bc
            ld de,FILE_DESCRIPTOR.size
            add hl,de
            ld e,(hl)
            inc hl
            ld d,(hl)
            inc hl
            ld c,(hl)
            inc hl
            ld b,(hl)
            ; BC:DE — длина. Округление через (N-1)/size+1.
            ld a,b
            or c
            or d
            or e
            jp z,sizeClustersReturn
            ld a,d
            or e
            dec de
            jp nz,sizeNoBorrow
            dec bc
sizeNoBorrow:
            ld hl,(v_diskInfo.clusterBytes)
sizeShiftLoop:
            call div_hl_2
            ld a,h
            or l
            jp z,sizeShiftDone
            ld a,b
            or a
            rra
            ld b,a
            ld a,c
            rra
            ld c,a
            ld a,d
            rra
            ld d,a
            ld a,e
            rra
            ld e,a
            jp sizeShiftLoop
sizeShiftDone:
            ld a,b
            or c
            jp nz,sizeClustersBad
            inc de
            ld a,d
            or e
            jp z,sizeClustersBad
sizeClustersReturn:
            pop bc
            pop hl
            or a
            ret
sizeClustersBad:
            pop bc
            pop hl
            jp blockArgument
v_streamOffset: DW 0
v_streamRemaining: DW 0
v_streamPtr: DW 0
v_streamCluster: DW 0
v_streamMode: DB 0
v_streamChunk: DW 0
v_streamSector: DW 0 ; абсолютный LBA текущего сектора
v_streamWithin: DW 0

; DE=LBA. CF=0, если сектор присутствует в кэше, иначе CF=1.
; При попадании HL=слот кэша; DE сохранён, BC/AF разрушены. Проверяем грязные строки:
; прямое чтение не должно возвращать старую дисковую копию.
streamCacheContains:
            ld hl,cacheData
            ld a,(v_cacheCount)
            ld c,a
streamCacheProbe:
            call cmp_m_de
            ret z
            push bc
            ld bc,FAT_CACHE_DATA
            add hl,bc
            pop bc
            dec c
            jp nz,streamCacheProbe
            scf
            ret
; Слот уже найден: обновляем возраст/dirty без повторного поиска.
streamCachedHit:
            ld a,(v_streamMode)
            and 1
            ld b,a
            call cacheTouchSlot
            jp streamCachedAddress

; Конец полного файла после fatStream. DE=курсор кластера, смещение глобальное.
; HL/BC/DE сохранены, CF=ошибка. Нулевая цепочка допустима для пустого файла.
fatStreamFinish:
            push hl
            push de
            ld a,d
            or e
            jp z,streamFinishOk
            ld hl,(v_streamOffset)
            ld a,h
            or l
            jp z,streamFinishCheck
            call fatReadCluster
            jp c,streamFinishReturn
streamFinishCheck:
            ld a,d
            cp 0FFh
            jp nz,streamFinishBad
            ld a,e
            cp 0F8h
            jp c,streamFinishBad
streamFinishOk:
            xor a
streamFinishReturn:
            pop de
            pop hl
            ret
streamFinishBad:
            call dosCorrupt
            jp streamFinishReturn

; HL/BC/DE сохранены, CF=ошибка. Обычная память либо резидентная утилита.
; DOS, драйверы, кэш и аппаратные области в разрешённые окна не входят.
streamBufferRange:
            ; Локальные буферы C располагаются в выделенном системном стеке.
            ; Активные кадры вызовов не должны пересекаться с буфером.
            push de
            ld de,STACK_BOTTOM
            call cmp_hl_de
            pop de
            jp c,streamResidentRange
            push de
            ld de,STACK_ADDR
            call cmp_hl_de
            pop de
            jp nc,streamResidentRange
            push hl
            push de
            ld de,STACK_ADDR
            jp streamCheckUpper
streamResidentRange:
            push de
            ld de,SYSTEM_PROGRAM_ADDR
            call cmp_hl_de
            pop de
            jp c,fileOpRange
            push hl
            push de
            ld de,SYSTEM_PROGRAM_END
streamCheckUpper:
            add hl,bc
            jp c,streamRangeBad
            call cmp_hl_de
            jp c,streamRangeGood
            jp z,streamRangeGood
streamRangeBad:
            pop de
            pop hl
            jp blockArgument
streamRangeGood:
            pop de
            pop hl
            or a
            ret
