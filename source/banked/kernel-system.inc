; Физический обмен выбранного диска. Только данные и описания пересекают
; банк; адрес аппаратного обработчика никогда не выдаётся клиенту.
dosRawIO:
    jp j_diskDriver
krRaw:
    ld a,(kdRegs+5)
    ld (krOperation),a
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
    call kdRange
    jp c,kdBad
    call klEligible
    jp nz,krBuffered
    ld a,(krOperation)
    cp 5
    jp z,krBuffered
    ld hl,klBlock
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
    ld de,0FB80h
    jp krStoreInfo
krInfo:
    ld b,0FFh
    call j_diskDriver
    call kdCapture
    ld hl,(kdRegs)
    ld bc,64
    ld de,0FBC0h
krStoreInfo:
    ; Стабильная копия до следующего RAWIO; окно ROM-моста её не затирает.
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
    ld hl,0FB00h
    ld (kdCopy+4),hl
    ld a,DOS_IO_PAGE
    ld (kdCopy),a
    ld a,(kdPage)
    ld (kdCopy+3),a
    ld bc,FILE_COPY_REQUEST
    call kdCopyCount
    ret c
    ld hl,0FB00h
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

; Исторические FILECREATE/PREPARE используют тот же поток, что FileHandle.
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
krSectorSize: DW 0

; A=7 (H:), C=0 ROM / 1 FLASH / 2 FLASHFST. Установленные копии находятся
; в слотах DOS, COM не передаёт указатель на свой код. CF/A=ошибка DOS;
; AF/BC/DE/HL разрушены. Диск и каталоги вызывающей стороны сохраняются.
dosDriverSelect:
    cp 7
    jp nz,blockArgument
    ld a,c
    cp 3
    jp nc,blockArgument
    ld hl,kernelHDriverKind
    cp (hl)
    jp z,dosClearError
    push bc
    call flushVolume
    pop bc
    ret c
    ld a,(kernelHDriverKind)
    cp 2
    jp nz,krDriverReady
    push bc
    ld b,4
    call fastDriver
    pop bc
    ret c
krDriverReady:
    ld a,c
    ld (krDriverChoice),a
    ld hl,v_diskInfo
    ld (romDiskInfoPointer),hl
    or a
    ld hl,romDriver
    jp z,krDriverInstall
    dec a
    jp nz,krDriverFast
    call flashDosInit
    ret c
    ld hl,v_diskInfo
    ld (FuncSize+4),hl
    ld hl,Driver
    jp krDriverInstall
krDriverFast:
    call fastDosInit
    ret c
    ld hl,v_diskInfo
    ld (fastFuncSize+4),hl
    xor a
    ld hl,fastChipStates
    ld b,16
krDriverReset:
    ld (hl),a
    inc hl
    dec b
    jp nz,krDriverReset
    ld hl,fastDriver
krDriverInstall:
    ld (v_drives+14),hl
    ld a,(krDriverChoice)
    ld (kernelHDriverKind),a
    ld a,(v_drive)
    cp 7
    jp nz,dosClearError
    ld (j_diskDriver+1),hl
    jp mountVolume
kernelHDriverKind: DB 1
krDriverChoice: DB 0
