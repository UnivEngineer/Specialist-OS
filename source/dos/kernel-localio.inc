; B=RAW_OP_PREPARE: HL=единый локальный вход 1/2/4/5, DE=0,
; B=blockShift, C=диск. Указатели действуют до следующего входа в DOS.
; Не проверяют буферы/LBA: это доверенный низкоуровневый интерфейс.
dosRawPrepare:
    call klEligible
    jp nz,dosUnsupported
    call flushVolume
    ret c
    ld a,(v_drive)
    cp 7
    jp nz,klPrepareChecked
    call klInstall
    ret c
klPrepareChecked:
    ld hl,LOCAL_BLOCK_ENTRY
    ld de,0
    ld a,(v_drive)
    ld c,a
    ld b,8
    xor a
    ret

; Полный блок передаётся в память клиента без секторного буфера DOS.
; HL=NEAR клиента, DE=блок, B=1/2/5; BC/DE/HL сохраняются.
klBlock:
    push bc
    push de
    push hl
    ld (klRegs),hl
    ex de,hl
    ld (klRegs+2),hl
    ld a,(v_drive)
    ld c,a
    ld h,b
    ld l,c
    ld (klRegs+4),hl
    or a
    jp nz,klRamBounds
    ld hl,(DOS_DRIVER_REGISTRY)
    ld a,(klRegs+3)
    cp h
    jp c,klNotRam
    jp nz,klBad
    ld a,(klRegs+2)
    cp l
    jp nc,klBad
    jp klNotRam
klRamBounds:
    ld a,c
    cp 1
    jp nz,klNotRam
    ld a,(klRegs+2)
    cp 0FFh
    jp z,klBad
    ld a,(memoryRamPages)
    ld hl,klRegs+3
    cp (hl)
    jp c,klBad
    jp z,klBad
    ld a,(klRegs+5)
    cp 5
    jp nz,klRamDirect
klNotRam:
    ld a,c
    cp 7
    jp nz,klInvoke
    call klInstall
    jp c,klDone
klInvoke:
    ld bc,LOCAL_BLOCK_ENTRY
klInvokePage:
    ld a,(kdPage)
    ld hl,bankCallback
    ld de,klRegs
    call bankCallPacket
klDone:
    pop hl
    pop de
    pop bc
    ret
 ; RAM-диск: копировщик напрямую связывает носитель и страницу буфера.
; Не нужны callback, стек клиента и исполнение драйвера в его странице.
klRamDirect:
    ld a,(klRegs+3)
    add a,2
    ld (klRamCopy),a
    ld a,(klRegs+2)
    ld h,a
    ld l,0
    ld (klRamCopy+1),hl
    ld a,(kdPage)
    ld (klRamCopy+3),a
    ld hl,(klRegs)
    ld (klRamCopy+4),hl
    ld a,(klRegs+5)
    dec a
    jp nz,klRamRead
    ; WRITE: поменять местами источник и приёмник.
    ld a,(klRamCopy)
    ld hl,klRamCopy+3
    ld e,(hl)
    ld (hl),a
    ld a,e
    ld (klRamCopy),a
    ld hl,(klRamCopy+1)
    ex de,hl
    ld hl,(klRamCopy+4)
    ld (klRamCopy+1),hl
    ex de,hl
    ld (klRamCopy+4),hl
klRamRead:
    ld hl,klRamCopy
    call bankCopySector
    jp klDone
klRamCopy: DB 0
    DW 0
    DB 0
    DW 0,256
klBad:
    ld a,DOS_ERR_ARGUMENT
    scf
    jp klDone

; Все существующие физические драйверы A:/B:/H:, любой EXEC-клиент.
; Выход Z=прямой путь, AF разрушен. Геометрия остальных устройств не меняется.
klEligible:
    ld a,(v_diskInfo.blockShift)
    cp 8
    ret nz
    ld a,(v_drive)
    cp 1
    ret z
    or a
    ret z
    cp 7
    ret nz
    push hl
    ld hl,(DOS_DRIVER_REGISTRY+7*DRIVER_RECORD_SIZE)
    ld a,h
    or l
    pop hl
    jp nz,klEligibleH
    inc a
    ret
klEligibleH:
    xor a
    ret

; У каждой страницы своя отметка установленной копии. После нового профиля
; EXEC локальный маркер сброшен, даже если FAR-номер банка использован повторно.
klInstall:
    ld a,(kdPage)
    call klTakeOwner
    ret c
    ld a,(klRequestReady)
    or a
    ret nz
    ld a,(kdPage)
    ld e,a
    ld d,0
    ld hl,klInstalled
    add hl,de
    ld (klMarker),hl
    ld a,(kernelHDriverKind)
    inc a
    cp (hl)
    jp nz,klCopy
    ld hl,LOCAL_DRIVER_KIND
    ld de,klProbe
    ld bc,1
    call kdFetch
    ret c
    ld a,(klProbe)
    ld hl,(klMarker)
    cp (hl)
    jp z,klRequestInstalled
klCopy:
    ld hl,(registryImageSize)
    ld b,h
    ld c,l
    ld hl,DOS_DRIVER_IMAGE
    ld de,LOCAL_DRIVER_ADDR
    call klStoreDriver
    ret c
    ld a,(kernelHDriverKind)
    inc a
    ld (klProbe),a
    ld hl,klProbe
    ld de,LOCAL_DRIVER_KIND
    ld bc,1
    call kdStore
    ret c
    ld a,(klProbe)
    ld hl,(klMarker)
    ld (hl),a
klRequestInstalled:
    ld a,1
    ld (klRequestReady),a
    or a
    ret
klRequestReady: DB 0
; Внутренняя установка в фиксированный служебный слот клиента. Публичный
; kdStore правильно запрещает FA00h в EXEC; здесь размер проверен реестром.
klStoreDriver:
    ld (kdCopy+1),hl
    ex de,hl
    ld (kdCopy+4),hl
    ld a,DOS_IO_PAGE
    ld (kdCopy),a
    ld a,(kdPage)
    ld (kdCopy+3),a
    jp kdCopyCount
klInvalidate:
    ld hl,klInstalled
    ld b,17
    xor a
klInvalidateNext:
    ld (hl),a
    inc hl
    dec b
    jp nz,klInvalidateNext
    ret
klInstalled: DS 17,0
klMarker: DW 0
klProbe: DB 0
klRegs: DS MX_REGS,0

; Владелец конвейера — FAR-код страницы, DOS=1, MAIN=0.
; Смена владельца ждёт старую копию; последующие сектора одного клиента
; продолжают конвейер. Данные секторов через DOS при этом не пересылаются.
klTakeOwner:
    ld e,a
    ld a,(kernelHDriverKind)
    cp 2
    jp nz,klOwnerReady
    ld a,(klFastOwner)
    cp e
    jp z,klOwnerReady
    push de
    cp DOS_IO_PAGE
    jp nz,klFlushClient
    ld b,4
    call LOCAL_DRIVER_ADDR
    jp klOwnerDone
klFlushClient:
    ld bc,LOCAL_BLOCK_ENTRY
    ld hl,bankCallback
    ld de,klFlushRegs
    call bankCallPacket
klOwnerDone:
    pop de
    ret c
    ld a,e
    ld (klFastOwner),a
klOwnerReady:
    xor a
    ret
klReclaim:
    ld a,DOS_IO_PAGE
    jp klTakeOwner
fastDriver:
    push bc
    push de
    push hl
    call klReclaim
    pop hl
    pop de
    pop bc
    ret c
    jp LOCAL_DRIVER_ADDR
klFastOwner: DB DOS_IO_PAGE
klFlushRegs: DW 0,0,0407h,0

; Прямая серия RAW не требует перехода main -> DOS на каждом блоке.
; Пакет результата уже сохранён в kdRegs; AF/BC/DE/HL разрушены.
klGrant:
    ld a,(kdPage)
    or a
    ret nz
    ld a,(kdRegs+6)
    and 1
    ret nz
    ld a,(v_drive)
    push af
    call registryRecord
    ld e,(hl)
    inc hl
    ld d,(hl)
    ex de,hl
    ld (klLease+1),hl
    pop af
    inc a
    ld (klLease),a
    ld hl,klLease
    ld de,MAIN_RAW_LEASE
    ld bc,3
    jp kdStore
klLease: DS 3,0
