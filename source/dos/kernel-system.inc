; Физический обмен выбранного диска. Обычный запрос переносит параметры;
; B=RAW_OP_PREPARE выдаёт точки в странице клиента для прямой серии.
dosRawIO:
    ld a,b
    cp RAW_OP_PREPARE
    jp z,dosRawPrepare
    cp 5
    jp nz,j_diskDriver
    push bc
    ld bc,j_diskDriver
    call dosVerify.diskWriteChecked
    pop bc
    ret
    MODULE dosVerify
    DEFINE DWC_NO_ERROR_FLAG
    DEFINE DWC_READ_BUFFER DOS_VERIFY_BUFFER
    INCLUDE "../common/diskWriteChecked.inc"
    UNDEFINE DWC_READ_BUFFER
    UNDEFINE DWC_NO_ERROR_FLAG
    ENDMODULE
krRaw:
    ld a,(kdRegs+5)
    ld (krOperation),a
    cp RAW_OP_PREPARE
    jp z,krPrepareLocal
    cp 3
    jp z,krSize
    cp 0FFh
    jp z,krInfo
    cp 4
    jp z,krFlush
    cp 5
    jp z,krRawBuffer
    dec a
    cp 2
    jp nc,kdBad
krRawBuffer:
    ld hl,(kdRegs)
    ld (kdClientObject),hl
    call krBlockSize
    call kiRange
    jp c,kdBad
    call klEligible
    jp nz,krBuffered
    ld hl,klBlock
    ld (kdEntry),hl
    call kdInvoke
    call kdCapture
    call klGrant
    jp kdFinish
; Подготовить маршрут один раз, до запуска секторного цикла клиента.
krPrepareLocal:
    ld hl,dosRawPrepare
    ld (kdEntry),hl
    call kdInvoke
    call kdCapture
    jp kdFinish
krBuffered:
    ld a,(kdRegs+5)
    cp 2
    jp z,krRead
    ld de,KD_DATA
    call kdFetch
    jp c,kdBad
krRead:
    ld a,(krOperation)
    cp 5
    jp nz,krInvoke
    ld a,1
    ld (kdRegs+5),a
krInvoke:
    ld hl,j_diskDriver
    ld (kdEntry),hl
    ld hl,KD_DATA
    ld (kdRegs),hl
    call kdInvoke
    call kdCapture
    ld a,(kdRegs+6)
    and 1
    jp nz,krRawDone
    ld a,(krOperation)
    cp 5
    jp z,krVerify
    ld a,(kdRegs+5)
    cp 2
    jp nz,krRawDone
    ld hl,(kdClientObject)
    ex de,hl
    ld hl,KD_DATA
    call krBlockSize
    call kdStore
    jp c,kdBad
    jp krRawDone
; B=5 обслуживается DOS одинаково для ROM/RAM/flash. Не требуется
; размещать BIOS-процедуру сравнения или её адрес в физическом драйвере.
krVerify:
    ld hl,KD_DATA+512
    ld (kdRegs),hl
    ld a,2
    ld (kdRegs+5),a
    call kdInvoke
    call kdCapture
    ld a,(kdRegs+6)
    and 1
    jp nz,krRawDone
    ld hl,KD_DATA
    ld de,KD_DATA+512
    call krBlockSize
krCompare:
    ld a,(de)
    cp (hl)
    jp nz,krVerifyFailed
    inc hl
    inc de
    dec bc
    ld a,b
    or c
    jp nz,krCompare
    jp krRawDone
krVerifyFailed:
    ld a,DOS_ERR_IO
    call dosSetError
    push af
    pop hl
    ld (kdRegs+6),hl
krRawDone:
    ld a,(krOperation)
    ld (kdRegs+5),a
    jp kdRestorePointer
; Flush не содержит указателей и не требует копирования буфера.
krFlush:
    ld hl,j_diskDriver
    ld (kdEntry),hl
    call kdInvoke
    call kdCapture
    jp kdFinish
krOperation: DB 0
krSize:
    ld b,3
    call j_diskDriver
    call kdCapture
    ld hl,v_diskInfo
    ld bc,DISK_INFO
    ld de,MAIN_DISK_INFO
    jp krStoreInfo
krInfo:
    ld b,0FFh
    call j_diskDriver
    call kdCapture
    ld hl,(kdRegs)
    ld bc,64
    ld de,MAIN_DRIVER_INFO
krStoreInfo:
    ; Стабильная копия до следующего RAW_IO; окно ROM-моста её не затирает.
    push de
    call kdStore
    pop hl
    jp c,kdBad
    ld (kdRegs),hl
    jp kdFinish

krBlockSize:
    push hl
    ld hl,(v_diskInfo.blockSize)
    ld b,h
    ld c,l
    pop hl
    ret

; Текстовый COPY/MOVE использует снимок запроса в зарезервированном хвосте
; клиента. Обработчик читает его, но возвращает решение только в A/CF.
ktConfirm:
    ld de,KD_PATH
    ld bc,FILE_COPY_REQUEST
    call moveBytesForward
    ld hl,0
    ld (KD_PATH+FILE_COPY_REQUEST.buffer),hl
    ld (KD_PATH+FILE_COPY_REQUEST.bufferSize),hl
    ld hl,(ktHook)
    ld (KD_PATH+FILE_COPY_REQUEST.onExists),hl
    ld hl,KD_PATH
    ld (kdCopy+1),hl
    ld hl,CLIENT_COPY_SNAPSHOT
    ld (kdCopy+4),hl
    ld a,DOS_IO_PAGE
    ld (kdCopy),a
    ld a,(kdPage)
    ld (kdCopy+3),a
    ld bc,FILE_COPY_REQUEST
    call kdCopyCount
    ret c
    ld hl,CLIENT_COPY_SNAPSHOT
    ld (ktRegs),hl
    ld hl,(ktHook)
    ld b,h
    ld c,l
    ld a,(kdPage)
    ld hl,bankCallback
    ld de,ktRegs
    jp bankCallPacket
ktHook: DW 0
ktRegs: DS MX_REGS,0

; Исторические FILE_CREATE/PREPARE используют тот же поток, что FileHandle.
krPrepare:
    call kiFetchDescriptor
    jp c,kdBad
    ld hl,KD_OBJECT
    call fileCreatePrepare
    call kdCapture
    jp kiReturnDescriptor
krCreate:
    call kiFetchDescriptor
    jp c,kdBad
    ld hl,(KD_OBJECT+FILE_DESCRIPTOR.size+2)
    ld a,h
    or l
    jp nz,kdBad
    ld hl,(KD_OBJECT+FILE_DESCRIPTOR.size)
    ld b,h
    ld c,l
    ld hl,(kdRegs+2)
    call kdRange
    jp c,kdBad
    ld (kdFarCurrent),hl
    ld a,1
    ld (kdFarActive),a
    ld hl,KD_OBJECT
    ld de,KD_DATA
    call fileCreate
    call kdCapture
    xor a
    ld (kdFarActive),a
    jp kiReturnDescriptor

; Логический сектор: размер берётся из BPB текущего тома.
krSector:
    ld hl,(v_diskInfo.sectorSize)
    ld (krSectorSize),hl
    ld b,h
    ld c,l
    ld hl,(kdRegs)
    ld (kdClientObject),hl
    call kdRange
    jp c,kdBad
    call klEligible
    jp nz,krSectorBuffered
    ld hl,krSectorDirect
    ld (kdEntry),hl
    call kdInvoke
    call kdCapture
    jp kdFinish
krSectorBuffered:
    ld a,(kdRegs+5)
    cp 1
    jp nz,krSectorRead
    ld de,KD_DATA
    call kdFetch
    jp c,kdBad
krSectorRead:
    ld hl,logicalSectorIO
    ld (kdEntry),hl
    ld hl,KD_DATA
    ld (kdRegs),hl
    call kdInvoke
    call kdCapture
    ld a,(kdRegs+6)
    and 1
    jp nz,kdRestorePointer
    ld a,(kdRegs+5)
    cp 2
    jp nz,kdRestorePointer
    ld hl,(krSectorSize)
    ld b,h
    ld c,l
    ld hl,(kdClientObject)
    ex de,hl
    ld hl,KD_DATA
    call kdStore
    jp c,kdBad
    jp kdRestorePointer
; Полный логический сектор может состоять из нескольких физических блоков.
krSectorDirect:
    ld a,b
    dec a
    cp 2
    jp nc,blockArgument
    push hl
    ld hl,(v_logicalCapacity)
    call cmp_hl_de
    pop hl
    jp c,blockArgument
    jp z,blockArgument
    ld a,1
    ld (ksDirect),a
    call ksSector
    push af
    xor a
    ld (ksDirect),a
    pop af
    ret
krSectorSize: DW 0

 ; Старый выбор встроенных шаблонов больше не используется: запускается COM.
dosDriverSelect:
    jp dosUnsupported
kernelHDriverKind: DB 0
