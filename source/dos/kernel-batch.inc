; Полные 256-байтные сектора: один обход FAT собирает локальную пачку LBA.
; Прямой планировщик проверяет цепочку, расширяет запись штатным HANDLE
; и согласует кэш. Неполные сектора обслуживает общий поток.
sbPacket = DOS_IO_BATCH_ADDR
sbCount = sbPacket+4
sbList = sbPacket+6
sbCheckpoint = sbList+128
sbLimit = sbCheckpoint+8
sbNext = sbLimit+1
sbDone = sbNext+2
sbSpan = sbDone+1
sbSector = sbSpan+1
sbPrevious = sbSector+2
    ASSERT sbPrevious+2 <= DOS_CACHE_BOOT_ADDR

streamBatchStart:
    ld hl,(v_streamPrevious)
    ld (sbPrevious),hl
    ld hl,v_streamOffset
    ld de,sbCheckpoint
    ld bc,8
    call moveBytesForward
    ld a,DOS_IO_BATCH_SECTORS
    ld (sbLimit),a
    call sbPlan
    push af                     ; Ошибка FAT после подготовленного префикса.
    ld hl,(sbCheckpoint+4)
    ld (sbPacket+2),hl
    ld a,(v_streamMode)
    cp 1
    jp nz,sbOperation
    ld a,(v_streamChecked)
    or a
    ld a,1
    jp z,sbOperation
    ld a,5
sbOperation:
    ld b,a
    ld a,(v_drive)
    ld c,a
    ld h,b
    ld l,c
    ld (sbPacket),hl
    call sbExecute
    jp c,sbFailed
    pop af
    jp c,streamReturn
    jp streamLoop
sbFailed:
    push af
    ld a,c
    ld (sbLimit),a
    ld hl,(sbPrevious)
    ld (v_streamPrevious),hl
    ld hl,sbCheckpoint
    ld de,v_streamOffset
    ld bc,8
    call moveBytesForward
    ld a,(sbLimit)
    or a
    call nz,sbPlan              ; Восстановить курсор до успешного префикса.
    pop af
    pop bc                     ; Отбросить статус планирования, оставить I/O.
    jp streamReturn
sbPlan:
    xor a
    ld (sbCount),a
    ld (sbCount+1),a
    ld hl,sbList
    ld (sbNext),hl
sbPlanNext:
    ; Курсор после последнего полного сектора должен указывать уже на
    ; следующий кластер, даже если пачка/запрос закончились на границе.
    ld hl,(v_diskInfo.clusterBytes)
    ex de,hl
    ld hl,(v_streamOffset)
    call cmp_hl_de
    jp nz,sbPlanHaveCluster
    call streamAdvanceCluster
    ret c
sbPlanHaveCluster:
    ld a,(sbLimit)
    ld hl,sbCount
    cp (hl)
    ret z
    ld hl,(v_streamRemaining)
    ld a,h
    or a
    ret z                       ; Остаток 0..255 обслуживает общий поток.
    ld hl,(v_streamCluster)
    ex de,hl
    call streamEnsureWritable
    ret c
    call fatCheckCluster
    ret c
    call fatClusterToSector
    ld a,(v_streamOffset+1)     ; Здесь логический сектор ровно 256 байт.
    add a,e
    ld e,a
    jp nc,sbPlanSector
    inc d
sbPlanSector:
    push hl
    ex de,hl
    ld (v_streamSector),hl
    ex de,hl
    pop hl
    call streamCacheContains
    jp c,sbPlanUncached
    call streamDirectCached    ; Сохранить dirty-копию до прямого обмена.
    ret c
sbPlanUncached:
    ld hl,(sbNext)
    ld (hl),e
    inc hl
    ld (hl),d
    inc hl
    ld (sbNext),hl
    ld hl,sbCount
    inc (hl)
    ld hl,(v_streamRemaining)
    dec h
    ld (v_streamRemaining),hl
    ld hl,(v_streamPtr)
    inc h
    ld (v_streamPtr),hl
    ld hl,(v_streamOffset)
    inc h
    ld (v_streamOffset),hl
    jp sbPlanNext

; Установить маршрут один раз, затем исполнить весь список в банке буфера.
; В DOS вызов локальный; в MAIN/EXEC один bankCall на всю пачку.
sbExecute:
    ld a,(kdPage)
    push af
    ld a,(ksDirect)
    or a
    jp nz,sbHavePage
    ld a,DOS_IO_PAGE
    ld (kdPage),a
sbHavePage:
    ld a,(v_drive)
    cp 7
    jp nz,sbInstalled
    call klInstall
    jp c,sbInstallFailed
sbInstalled:
    ld a,(v_drive)
    cp 1
    jp z,sbRamCandidate
    or a
    jp nz,sbLocalWorker
    ld a,(sbPacket+1)
    cp 2
    jp nz,sbLocalWorker
    ld a,(kdPage)
    or a
    jp nz,sbCopyStart
    ld a,(sbPacket+3)
    or a
    jp m,sbLocalWorker           ; MAIN HIGH: отображённый near memcpy быстрее.
    jp sbCopyStart
sbRamCandidate:
    ld a,(sbPacket+1)
    cp 5
    jp z,sbLocalWorker
sbCopyStart:
    call sbCopyRuns
    jp sbExecuted
sbLocalWorker:
    ld a,(sbCount)
    add a,a
    add a,6
    ld c,a
    ld b,0
    ld de,sbPacket
    ld a,(kdPage)
    cp DOS_IO_PAGE
    jp nz,sbCallPage
    ld a,DOS_IO_PAGE
    call MEM_BATCH_ENTRY
    jp sbExecuted
sbCallPage:
    ld hl,MEM_BATCH_ENTRY
    call bankCall
sbExecuted:
    push af
    pop de
    pop af
    ld (kdPage),a
    push de
    pop af
    ret
sbInstallFailed:
    ld bc,0
    jp sbExecuted

; A: READ и B: READ/WRITE объединяют соседние LBA в один CPM32-перенос.
; Граница ROM-окна 32 КБ / страницы RAM 64 КБ всегда разрывает серию.
sbCopyRuns:
    xor a
    ld (sbDone),a
    ld a,(v_drive)
    or a
    ld hl,07FE6h             ; AND 7Fh, затем JP Z для ROM.
    ld a,0CAh
    jp z,sbMaskReady
    ld hl,(RAMDISK_DATA_BLOCK_LIMIT << 8)+0FEh ; CP FEh / JP NC для RAM.
    ld a,0D2h
sbMaskReady:
    ld (sbBoundary),hl
    ld (sbBoundaryJump),a
    ld hl,sbList
sbRunNext:
    ld e,(hl)
    inc hl
    ld d,(hl)
    inc hl
    push hl
    ex de,hl
    ld (sbSector),hl
    ex de,hl
    pop hl
    ld a,(sbCount)
    ld b,a
    dec b
    ld c,1
    inc de
sbRunScan:
    ld a,b
    or a
    jp z,sbRunReady
    ld a,e
sbBoundary:
    and 07Fh
sbBoundaryJump:
    jp z,sbRunReady
    ld a,(hl)
    cp e
    jp nz,sbRunReady
    inc hl
    ld a,(hl)
    dec hl
    cp d
    jp nz,sbRunReady
    inc hl
    inc hl
    inc de
    inc c
    dec b
    jp sbRunScan
sbRunReady:
    ld (sbNext),hl
    ld a,b
    ld (sbCount),a
    ld a,c
    ld (sbSpan),a
    ld h,a
    ld l,0
    ld (klRamCopy+6),hl
    ld hl,(sbSector)
    ex de,hl
    ld hl,(sbPacket+2)
    ld a,(sbPacket+1)
    ld b,a
    call klBlock
    jp c,sbRunsResult
    ld a,(sbSpan)
    add a,h
    ld h,a
    ld (sbPacket+2),hl
    ld a,(sbSpan)
    ld hl,sbDone
    add a,(hl)
    ld (hl),a
    ld hl,(sbNext)
    ld a,(sbCount)
    or a
    jp nz,sbRunNext
sbRunsResult:
    push af
    ld hl,256
    ld (klRamCopy+6),hl
    ld a,(sbDone)
    ld c,a
    ld b,0
    pop af
    ret
