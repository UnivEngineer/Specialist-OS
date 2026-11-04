; Открытые файлы: A=FILE_OP_*, HL=FILE_HANDLE в памяти вызывающего.
; Все параметры и результаты, кроме CF/A, находятся в структуре.
; AF/BC/DE/HL разрушены. Нереентерабельно; активным остаётся диск файла.
fileHandle:
            ld (v_handleOp),a
            ld (v_handle),hl
            ld bc,FILE_HANDLE
            call streamBufferRange
            jp nc,handleRangeOk
            ; Недопустимый объект не трогаем даже ради записи кода ошибки.
            ld a,(v_dosError)
            scf
            ret
handleRangeOk:
            xor a
            ld (v_dosError),a
            ld (bios_vars.diskWriteErr),a
            ld hl,0
            ld (v_handleNew),hl
            ld a,FH_DRIVE
            call handleField
            ld a,(hl)
            call fileChangeDrive
            jp c,handleExit
            ld hl,(v_volumeTable)
            ld de,fatVolumeDriver+VOLUME_DRIVER_HEADER
            call cmp_hl_de
            jp z,handleVolumeReady
            ld a,DOS_ERR_UNSUPPORTED
            call dosSetError
            jp handleExit
handleVolumeReady:
            ld a,(v_handleOp)
            or a
            jp z,handleOpen
            cp FILE_OP_CLOSE+1
            jp nc,handleArgument
            ld a,FH_STATE
            call handleField
            ld a,(hl)
            cp 046h
            jp nz,handleArgument
            ld a,(v_handleOp)
            dec a
            jp z,handleRead
            dec a
            jp z,handleWrite
            dec a
            jp z,handleSeek
            call handleFlush
            jp c,handleExit
            ld a,(v_handleOp)
            cp FILE_OP_CLOSE
            jp nz,handleSuccess
            ld a,FH_STATE
            call handleField
            ld (hl),0
handleSuccess:
            or a
            jp handleExit

; A=смещение поля, HL=его адрес; флаги разрушены, BC/DE/A сохранены.
handleField:
            push de
            ld hl,(v_handle)
            ld e,a
            ld d,0
            add hl,de
            pop de
            ret
handleArgument:
            call blockArgument
handleExit:
            call c,dosIoError
            ld a,(v_dosError)
            or a
            ret z
            push af
            call handleRollback
            ld a,FH_ERROR
            call handleField
            pop af
            ld (hl),a
            ld (v_dosError),a
            scf
            ret
handleDenied:
            ld a,DOS_ERR_ACCESS
            call dosSetError
            jp handleExit

handleOpen:
            ld a,FH_STATE
            call handleField
            ld a,(hl)
            cp 046h
            jp z,handleArgument
            ld a,FH_MODE
            call handleField
            ld a,(hl)
            and 0E0h
            jp nz,handleArgument
            ld a,(hl)
            and 3
            jp z,handleArgument
            ld a,(hl)
            and FILE_MODE_WRITE
            jp nz,handleOpenModeOk
            ld a,(hl)
            and 01Ch
            jp nz,handleArgument
handleOpenModeOk:
            ld a,FH_STATE
            call handleField
            ld (hl),0
            inc hl
            ld (hl),0
            inc hl
            ld (hl),0
            ld hl,(v_handle)
            call fileFind
            jp nc,handleOpenFound
            ld a,(v_dosError)
            cp DOS_ERR_NOT_FOUND
            jp nz,handleExit
            ld a,FH_MODE
            call handleField
            ld a,(hl)
            and FILE_MODE_CREATE
            jp z,handleExit
            jp handleCreate
handleOpenFound:
            ld a,FH_MODE
            call handleField
            ld a,(hl)
            and FILE_MODE_WRITE
            jp z,handleOpenCopy
            ld a,(v_tmpFileDescr+FILE_DESCRIPTOR.attrib)
            and 1
            jp nz,handleDenied
            ld a,(hl)
            and FILE_MODE_TRUNCATE
            jp z,handleOpenCopy
handleCreate:
            ; Имя уже подготовлено вызывающим: 11 байт 8.3 без преобразования KOI.
            ld a,11
            call handleField
            ex de,hl
            ld b,21
            xor a
            call memset_de_a_b
            ld hl,(v_handle)
            call fatFileCreatePrepare
            jp c,handleExit
            ld hl,(v_handle)
            call fileFind
            jp c,handleExit
handleOpenCopy:
            ld hl,(v_tmpFileDescr+FILE_DESCRIPTOR.firstClusterHi)
            ld a,h
            or l
            jp z,handleOpenClusterOk
            call dosCorrupt
            jp handleExit
handleOpenClusterOk:
            ld hl,(v_handle)
            ld de,v_tmpFileDescr
            call copyDescriptor
            ld a,FH_DIR_SECTOR
            call handleField
            ex de,hl
            ld hl,(v_cachedSector)
            ex de,hl
            ld (hl),e
            inc hl
            ld (hl),d
            inc hl
            ex de,hl
            ld hl,(v_cachedDescrOffset)
            ex de,hl
            ld (hl),e
            inc hl
            ld (hl),d
            ld a,FH_POSITION
            call handleField
            ex de,hl
            ld b,8
            xor a
            call memset_de_a_b
            ld a,FH_VALID
            call handleField
            ld (hl),0
            ld a,FH_STATE
            call handleField
            ld (hl),046h
            xor a
            ld (v_dosError),a
            jp handleExit

handleRead:
            ld b,FILE_MODE_READ
            jp handleIO
handleWrite:
            ld b,FILE_MODE_WRITE
handleIO:
            push bc
            ld a,FH_COUNT
            call handleLoadWord
            ld (v_handleLeft),hl
            ld a,FH_COUNT
            call handleField
            ld (hl),0
            inc hl
            ld (hl),0
            pop bc
            ld a,FH_MODE
            call handleField
            ld a,(hl)
            and b
            jp z,handleDenied
            ld hl,(v_handleLeft)
            ld b,h
            ld c,l
            ld a,FH_BUFFER
            call handleLoadWord
            ld (v_handleBuffer),hl
            call streamBufferRange
            jp c,handleExit
            ; Пересечение буфера с дескриптором запрещено.
            push hl
            ld hl,(v_handle)
            ex de,hl
            pop hl
            add hl,bc
            call cmp_hl_de
            jp c,handleBufferOk
            jp z,handleBufferOk
            ld a,FILE_HANDLE
            call handleField
            ex de,hl
            ld hl,(v_handleBuffer)
            call cmp_hl_de
            jp c,handleArgument
handleBufferOk:
            ld hl,(v_handleLeft)
            ld a,h
            or l
            jp z,handleExit
            xor a
            ld (v_handleEOF),a
            ld a,(v_handleOp)
            cp FILE_OP_WRITE
            jp nz,handleIOLoop
            ld a,FH_MODE
            call handleField
            ld a,(hl)
            and FILE_MODE_APPEND
            jp z,handleIOLoop
            ; Каждый append пишет в конец, независимо от предшествующего seek.
            ld a,FILE_DESCRIPTOR.size
            call handleField
            ex de,hl
            ld a,FH_POSITION
            call handleField
            call handleCopy4
            ld a,FH_VALID
            call handleField
            ld (hl),0
handleIOLoop:
            ld hl,(v_handleLeft)
            ld a,h
            or l
            jp z,handleIOEnd
            ; Чтение ограничено размером. Сначала вычисляем size-position.
            ld a,FH_POSITION
            call handleField
            ex de,hl
            ld a,FILE_DESCRIPTOR.size
            call handleField
            call handleDifference
            push af
            ld a,(v_handleOp)
            cp FILE_OP_WRITE
            jp z,handleWritePop
            pop af
            jp c,handleEOF
            ld hl,(v_handleDelta+2)
            ex de,hl
            ld hl,(v_handleDelta)
            ld a,d
            or e
            jp nz,handleReadCountOk
            ld a,h
            or l
            jp z,handleEOF
            ex de,hl
            ld hl,(v_handleLeft)
            call cmp_hl_de
            jp c,handleReadCountOk
            jp z,handleReadCountOk
            ld a,1
            ld (v_handleEOF),a
            ex de,hl
            ld (v_handleLeft),hl
handleReadCountOk:
            call handleLocate
            jp c,handleExit
            jp handleTransfer
handleWritePop:
            pop af
handleWritePosition:
            ; В C промежуток после seek за EOF имеет неопределённое содержимое.
            ; Кластеры промежутка выделяются здесь, до первой записи данных.
            call handleLocate
            jp c,handleExit
handleTransfer:
            ld a,FH_CURSOR
            call handleLoadWord
            push hl
            ld a,FH_PREVIOUS
            call handleField
            pop de
            ld (hl),e
            inc hl
            ld (hl),d
            ; Курсор сохраняется между вызовами; переход FAT — один на кластер.
            ld a,FH_CURSOR+2
            call handleLoadWord
            ex de,hl
            ld hl,(v_diskInfo.clusterBytes)
            call sub_hl_de
            push hl
            ld hl,(v_handleLeft)
            ex de,hl
            pop hl
            call cmp_hl_de
            jp c,handleChunkReady
            ex de,hl
handleChunkReady:
            ld (v_handleChunk),hl
            ld b,h
            ld c,l
            ld a,FH_CURSOR+2
            call handleLoadWord
            ld (v_streamOffset),hl
            ld a,FH_CURSOR
            call handleLoadWord
            ex de,hl
            ld hl,(v_handleBuffer)
            ld a,(v_handleOp)
            xor 3                 ; FILE_OP_READ=1 -> 2, WRITE=2 -> 1.
            ; Буфер/объект проверены один раз на весь запрос: повторная
            ; проверка публичного FileStream для каждого кластера не нужна.
            call fatStream
            push af
            ld (v_handleBuffer),hl
            ld a,FH_CURSOR
            call handleStoreWord
            ld hl,(v_streamOffset)
            ex de,hl
            ld a,FH_CURSOR+2
            call handleStoreWord
            ld hl,(v_streamRemaining)
            ex de,hl
            ld hl,(v_handleChunk)
            call sub_hl_de
            ld b,h
            ld c,l
            push hl
            ld h,b
            ld l,c
            ld (v_handleDone),hl
            pop hl
            ld a,b
            or c
            jp z,handleNoCommit
            push hl
            ld hl,0
            ld (v_handleNew),hl
            pop hl
handleNoCommit:
            ; Обновляем позицию и число байтов даже при частичном отказе.
            ld a,FH_POSITION
            call handleField
            call handleAddCount
            ld a,FH_COUNT
            call handleField
            ld e,(hl)
            inc hl
            ld d,(hl)
            ex de,hl
            add hl,bc
            ex de,hl
            ld (hl),d
            dec hl
            ld (hl),e
            ld hl,(v_handleLeft)
            ld d,b
            ld e,c
            call sub_hl_de
            ld (v_handleLeft),hl
            ld a,FH_POSITION
            call handleField
            ex de,hl
            ld a,FILE_DESCRIPTOR.size
            call handleField
            call handleDifference
            push af
            ld a,(v_handleOp)
            cp FILE_OP_WRITE
            jp nz,handleReadFinish
            pop af
            jp nc,handleTransferResult
            ld a,FH_POSITION
            call handleField
            ex de,hl
            ld a,FILE_DESCRIPTOR.size
            call handleField
            call handleCopy4
            jp handleTransferResult
handleReadFinish:
            pop af
            ld hl,(v_handleDelta)
            ld a,h
            or l
            ld hl,(v_handleDelta+2)
            or h
            or l
            jp nz,handleTransferResult
            ld a,FH_CURSOR
            call handleLoadWord
            ex de,hl
            call fatStreamFinish
            jp c,handleTransferFailed
handleTransferResult:
            pop af
            jp c,handleExit
            jp handleIOLoop
handleTransferFailed:
            pop af
            jp handleExit
handleEOF:
            ld a,FH_EOF
            call handleField
            ld (hl),1
            xor a
            ld (v_dosError),a
            jp handleExit
handleIOEnd:
            ld a,(v_handleEOF)
            or a
            jp nz,handleEOF
            jp handleExit

; Восстановить курсор после seek. Внутри последовательного обмена он готов.
handleLocate:
            ld a,FH_VALID
            call handleField
            ld a,(hl)
            or a
            jp nz,handleEnsureCluster
            ld a,FH_POSITION
            call handleField
            ex de,hl
            ld hl,v_handleDelta
            call handleCopy4
            ld a,FILE_DESCRIPTOR.firstCluster
            call handleLoadWord
            ld (v_handleCluster),hl
            ld hl,0
            ld (v_handlePrevious),hl
            ld hl,(v_diskInfo.dataClusters)
            inc hl
            inc hl
            ld (v_handleWalk),hl
handleLocateLoop:
            ld hl,(v_handleWalk)
            dec hl
            ld (v_handleWalk),hl
            ld a,h
            or l
            jp z,dosCorrupt
            call handleEnsureCurrent
            ret c
            ld hl,(v_handleDelta+2)
            ld a,h
            or l
            jp nz,handleLocateNext
            ld hl,(v_diskInfo.clusterBytes)
            ex de,hl
            ld hl,(v_handleDelta)
            call cmp_hl_de
            jp c,handleLocated
handleLocateNext:
            ld hl,(v_diskInfo.clusterBytes)
            ex de,hl
            ld hl,v_handleDelta
            ld bc,0
            call handleSubtractWord
            ld hl,(v_handleCluster)
            ld (v_handlePrevious),hl
            ex de,hl
            call fatReadCluster
            ret c
            ex de,hl
            ld (v_handleCluster),hl
            jp handleLocateLoop
handleLocated:
            ld a,FH_PREVIOUS
            call handleField
            ex de,hl
            ld hl,(v_handlePrevious)
            ex de,hl
            ld (hl),e
            inc hl
            ld (hl),d
            ld a,FH_CURSOR
            call handleField
            ex de,hl
            ld hl,(v_handleCluster)
            ex de,hl
            ld (hl),e
            inc hl
            ld (hl),d
            inc hl
            ex de,hl
            ld hl,(v_handleDelta)
            ex de,hl
            ld (hl),e
            inc hl
            ld (hl),d
            ld a,FH_VALID
            call handleField
            ld (hl),1
            ret
handleEnsureCluster:
            ld a,FH_PREVIOUS
            call handleLoadWord
            ld (v_handlePrevious),hl
            ld a,FH_CURSOR
            call handleLoadWord
            ld (v_handleCluster),hl
            call handleEnsureCurrent
            ret c
            ld a,FH_CURSOR
            call handleField
            ex de,hl
            ld hl,(v_handleCluster)
            ex de,hl
            ld (hl),e
            inc hl
            ld (hl),d
            ret
handleEnsureCurrent:
            ld hl,(v_handleCluster)
            ex de,hl
            ld a,d
            or e
            jp z,handleAllocate
            ld a,d
            cp 0FFh
            jp nz,fatCheckCluster
            ld a,e
            cp 0F8h
            jp c,fatCheckCluster
handleAllocate:
            ld a,(v_handleOp)
            cp FILE_OP_WRITE
            jp nz,dosCorrupt
            ld hl,(v_handlePrevious)
            ld a,h
            or l
            jp nz,handleAllocateNext
            call fatFindClusterFirst
            jp handleAllocateFound
handleAllocateNext:
            ld (v_findCluster),hl
            call fatFindClusterNext
handleAllocateFound:
            ret c
            push de
            ex de,hl
            ld de,0FFFFh
            call fatWriteCluster
            pop de
            ret c
            ld hl,(v_handleNew)
            ld a,h
            or l
            jp nz,handleAllocateTracked
            ex de,hl
            ld (v_handleNew),hl
            ex de,hl
            ld hl,(v_handlePrevious)
            ld (v_handleOldTail),hl
handleAllocateTracked:
            push de
            ld hl,(v_handlePrevious)
            ld a,h
            or l
            jp z,handleAllocateFirst
            call fatWriteCluster
            jp handleAllocateLinked
handleAllocateFirst:
            ld a,FILE_DESCRIPTOR.firstCluster
            call handleField
            pop de
            push de
            ld (hl),e
            inc hl
            ld (hl),d
handleAllocateLinked:
            pop hl
            ld (v_handleCluster),hl
            ret

; Не оставляем цепочку, выделенную для ещё не записанного промежутка/хвоста.
; Исходный код ошибки сохраняется; аппаратный отказ отката не транзакционен.
handleRollback:
            ld hl,(v_handleNew)
            ld a,h
            or l
            ret z
            ex de,hl
handleRollbackLoop:
            call fatCheckCluster
            jp c,handleRollbackEnd
            call fatFreeCluster
            jp c,handleRollbackEnd
            ld a,d
            cp 0FFh
            jp nz,handleRollbackLoop
            ld a,e
            cp 0F8h
            jp c,handleRollbackLoop
            ld hl,(v_handleOldTail)
            ld a,h
            or l
            jp z,handleRollbackFirst
            ld de,0FFFFh
            call fatWriteCluster
            jp handleRollbackEnd
handleRollbackFirst:
            ld a,FILE_DESCRIPTOR.firstCluster
            call handleField
            ld (hl),0
            inc hl
            ld (hl),0
handleRollbackEnd:
            ld a,FH_VALID
            call handleField
            ld (hl),0
            ret

; Seek использует signed int32 displacement, SET/CUR/END; файл не расширяет.
handleSeek:
            ld a,FH_ORIGIN
            call handleField
            ld a,(hl)
            cp 3
            jp nc,handleArgument
            or a
            jp z,handleSeekSet
            dec a
            ld a,FH_POSITION
            jp z,handleSeekBase
            ld a,FILE_DESCRIPTOR.size
handleSeekBase:
            call handleField
            ex de,hl
            ld a,FH_SEEK
            call handleField
            push hl
            ld bc,3
            add hl,bc
            ld a,(hl)
            and 080h
            ld (v_handleSign),a
            pop hl
            ld b,4
            or a
handleSeekAdd:
            ld a,(de)
            adc a,(hl)
            ld (hl),a
            inc hl
            inc de
            dec b
            jp nz,handleSeekAdd
            ld a,0
            adc a,0
            ld b,a
            ld a,(v_handleSign)
            rlca
            and 1
            cp b
            jp nz,handleArgument
            jp handleSeekSave
handleSeekSet:
            ld a,FH_SEEK+3
            call handleField
            ld a,(hl)
            and 080h
            jp nz,handleArgument
handleSeekSave:
            ld a,FH_SEEK
            call handleField
            ex de,hl
            ld a,FH_POSITION
            call handleField
            call handleCopy4
            ld a,FH_VALID
            call handleField
            ld (hl),0
            ld a,FH_EOF
            call handleField
            ld (hl),0
            jp handleExit

handleFlush:
            ld a,FH_MODE
            call handleField
            ld a,(hl)
            and FILE_MODE_WRITE
            ret z
            ; Сначала данные/FAT, потом публикация firstCluster и размера.
            call saveSectorCache
            ret c
            call fatFinishWrite
            ret c
            ld a,FH_DIR_SECTOR
            call handleLoadWord
            ld (v_cachedSector),hl
            ld a,FH_DIR_OFFSET
            call handleLoadWord
            ld (v_cachedDescrOffset),hl
            call markDirSectorAsModified
            ret c
            ; Остальные поля каталога не перезаписываем устаревшей копией.
            ld de,FILE_DESCRIPTOR.firstCluster
            add hl,de
            push hl
            ld a,FILE_DESCRIPTOR.firstCluster
            call handleField
            ex de,hl
            pop hl
            ld b,6
handleFlushCopy:
            ld a,(de)
            ld (hl),a
            inc de
            inc hl
            dec b
            jp nz,handleFlushCopy
            call saveSectorCache
            ret c
            jp fatFinishWrite

; Вспомогательная арифметика little-endian; не вызывает DOS.
handleStoreWord:
            call handleField
            ld (hl),e
            inc hl
            ld (hl),d
            ret
handleLoadWord:
            call handleField
            ld e,(hl)
            inc hl
            ld d,(hl)
            ex de,hl
            ret
handleCopy4:
            ld b,4
handleCopyLoop:
            ld a,(de)
            ld (hl),a
            inc de
            inc hl
            dec b
            jp nz,handleCopyLoop
            ret
; HL-DE -> v_handleDelta (32 бита), CF=заём; указатели разрушены.
handleDifference:
            ld bc,v_handleDelta
            or a
handleDifferenceLoop:
            ld a,(de)
            push bc
            ld b,a
            ld a,(hl)
            sbc a,b
            pop bc
            ld (bc),a
            inc hl
            inc de
            inc bc
            push af
            ld a,c
            cp (v_handleDelta+4)&0FFh
            jp z,handleDifferenceDone
            pop af
            jp handleDifferenceLoop
handleDifferenceDone:
            pop af
            ret
; HL=32-битный счётчик, BC=прибавляемые байты; BC сохранён.
handleAddCount:
            ld a,(hl)
            add a,c
            ld (hl),a
            inc hl
            ld a,(hl)
            adc a,b
            ld (hl),a
            ld b,2
handleAddHigh:
            inc hl
            ld a,(hl)
            adc a,0
            ld (hl),a
            dec b
            jp nz,handleAddHigh
            push hl
            ld hl,(v_handleDone)
            ld b,h
            ld c,l
            pop hl
            ret
; HL=число, DE=вычитаемое слово; AF/BC/DE/HL разрушены.
handleSubtractWord:
            ld a,(hl)
            sub e
            ld (hl),a
            inc hl
            ld a,(hl)
            sbc a,d
            ld (hl),a
            ld b,2
handleSubtractHigh:
            inc hl
            ld a,(hl)
            sbc a,0
            ld (hl),a
            dec b
            jp nz,handleSubtractHigh
            ret
v_handle: DW 0
v_handleOp: DB 0
v_handleLeft: DW 0
v_handleDone: DW 0
v_handleChunk: DW 0
v_handleBuffer: DW 0
v_handleDelta: DS 4,0
v_handleCluster: DW 0
v_handlePrevious: DW 0
v_handleSign: DB 0
v_handleEOF: DB 0
v_handleNew: DW 0
v_handleOldTail: DW 0
v_handleWalk: DW 0
