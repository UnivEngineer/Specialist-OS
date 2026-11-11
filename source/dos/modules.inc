; Модули: фиксированный контейнер COM/EXE, записи принадлежат DOS.
; Публичный вызов через MXOS_CALL; все NEAR-параметры находятся у kdPage.
; Запись: handle:word, active:byte, bank:byte, entry:word,
; allocation:word, owner:byte, allocationOwner:byte. ID не переиспользуются.
    MACRO MD_GET_DE address
    push hl
    ld hl,(address)
    ex de,hl
    pop hl
    ENDM
    MACRO MD_GET_BC address
    push hl
    ld hl,(address)
    ld b,h
    ld c,l
    pop hl
    ENDM
    MACRO MD_PUT_BC address
    push hl
    ld h,b
    ld l,c
    ld (address),hl
    pop hl
    ENDM
    MACRO MD_PUT_DE address
    ex de,hl
    ld (address),hl
    ex de,hl
    ENDM
moduleInit:
    ld hl,MODULE_TABLE
    ld b,MODULE_MAX*MODULE_RECORD_SIZE
    xor a
mdClear:
    ld (hl),a
    inc hl
    dec b
    jp nz,mdClear
    ld (moduleActor),a
    ld (moduleDosOwner),a
    ld (moduleDosBorrowed),a
    ld (moduleDosServiceOwner),a
    ld (appOwner),a
    ld (appDepth),a
    ld hl,residentKeys
    ld b,MODULE_MAX*2
mdClearResidents:
    ld (hl),a
    inc hl
    dec b
    jp nz,mdClearResidents
    ld hl,1
    ld (mdNext),hl
    ret

; Владелец выделений: модуль, временное MAIN-приложение или исходный банк.
; BC/DE/HL сохранены, AF разрушен. Это не идентификатор процесса.
memoryOwnerCurrent:
    ld a,(moduleActor)
    or a
    ret nz
    ld a,(appOwner)
    or a
    ret nz
    ld a,(kdPage)
    ret

; Reset отбрасывает стек вызывавшей программы: её модули больше не исполняемы.
; Освобождаем и активные записи, но не обнуляем счётчик поколений handle.
moduleWarm:
    xor a
    ld (moduleActor),a
    ld hl,MODULE_TABLE+2
    ld b,MODULE_MAX
mdResetActive:
    ld (hl),a
    ld de,MODULE_RECORD_SIZE
    add hl,de
    dec b
    jp nz,mdResetActive
    ld hl,MODULE_TABLE
    ld b,MODULE_MAX
mdResetModule:
    ld a,(hl)
    inc hl
    or (hl)
    dec hl
    jp z,mdResetNext
    call residentKey
    jp nz,mdResetNext
    push hl
    ld de,8
    add hl,de
    ld a,(hl)
    pop hl
    cp MODULE_OWNER_BASE
    jp nc,mdResetNext
    push bc
    push hl
    call mdRelease
    pop hl
    pop bc
mdResetNext:
    ld de,MODULE_RECORD_SIZE
    add hl,de
    dec b
    jp nz,mdResetModule
    ; Временные блоки main/обычных приложений не переживают потерю стека.
    ld hl,MEM_ALLOC_TABLE
    ld b,MEM_ALLOC_RECORDS
mdWarmBlocks:
    push hl
    ld de,5
    add hl,de
    ld a,(hl)
    pop hl
    cp DOS_IO_PAGE
    jp z,mdWarmBlockNext
    cp MODULE_OWNER_BASE
    jp nc,mdWarmBlockNext
    ld (hl),0FFh
mdWarmBlockNext:
    ld de,6
    add hl,de
    dec b
    jp nz,mdWarmBlocks
    xor a
    ld (appOwner),a
    ld (appDepth),a
    ld hl,0
    ld (memoryMainStart),hl
    ld (memoryMainEnd),hl
    ret

moduleLoad:
    xor a
    ld (mdBorrowRequest),a
    call dosClearError
    ld a,(kdRegs+7)
    cp MODULE_TARGET_DOS+1
    jp nc,mdRange
    ld (mdPlacement),a
    cp MODULE_TARGET_BANK
    jp z,mdResidentPlacementReady
    call residentActorCheck
    jp nz,residentDenied
mdResidentPlacementReady:
    ld a,(kdPage)
    ld (mdCaller),a
    call memoryOwnerCurrent
    ld (mdOwner),a
    ld hl,(mdNext)
    ld a,h
    or l
    jp z,mdNoMemory
    ld hl,MODULE_TABLE
    ld b,MODULE_MAX
    ld c,MODULE_OWNER_BASE
mdSlot:
    ld a,(hl)
    inc hl
    or (hl)
    dec hl
    jp z,mdSlotReady
    ld de,MODULE_RECORD_SIZE
    add hl,de
    inc c
    dec b
    jp nz,mdSlot
mdNoMemory:
    ld a,MEM_ERROR_NOMEM
    jp mdError
mdSlotReady:
    ld (mdRecord),hl
    ld de,6
    add hl,de
    xor a
    ld (hl),a
    inc hl
    ld (hl),a
    inc hl
    ld a,(mdOwner)
    ld (hl),a
    inc hl
    ld (hl),c
    ld hl,(kdRegs)
    ld de,KD_PATH
    ld bc,MAX_PATH
    call kdFetch
    jp c,kdBad
    ld hl,KD_PATH
    ld b,MAX_PATH
mdPath:
    ld a,(hl)
    or a
    jp z,mdOpen
    inc hl
    dec b
    jp nz,mdPath
    jp kdBad
mdOpen:
    ld hl,KD_PATH
    ld de,v_curFileDescr
    call fileNamePrepare
    jp c,mdFileError
    ld hl,v_curFileDescr
    call imageOpen
    jp c,mdFileError
    ld (mdEntry),hl
    ex de,hl
    ld (mdAddress),hl
    MD_PUT_BC mdBytes
    ld a,(mdPlacement)
    cp MODULE_TARGET_DOS
    jp z,mdDosImage
    ld a,(v_tmpFileDescr+5)
    cp IMAGE_FLAG_DRIVER
    jp z,mdImage
    or a
    jp z,mdPlacementReady
    cp IMAGE_FLAG_BANK
    ld a,(mdPlacement)
    jp z,mdNeedsBank
    or a
    jp nz,mdImage
    jp mdPlacementReady
mdNeedsBank:
    cp MODULE_TARGET_BANK
    jp nz,mdImage
    jp mdPlacementReady
mdDosImage:
    ld a,(v_tmpFileDescr+5)
    cp IMAGE_FLAG_DRIVER
    jp nz,mdImage
mdPlacementReady:
    add hl,bc
    ld a,(mdPlacement)
    cp MODULE_TARGET_DOS
    jp z,mdDos
    or a
    jp z,mdMain
    ld de,DOS_BANK_STACK_BOTTOM
    call cmp_hl_de
    jp c,mdBank
    jp nz,mdRange
mdBank:
    ; Весь DATA-банк резервируется целиком: чужие блоки исключены.
    ld hl,0FF00h
    ld b,MEM_ALLOC_ANY
    call memoryAlloc
    jp c,mdError
    jp mdReserved
mdDos:
    ; Резидентный слот не пересекается с временными DRIVER-инициализаторами.
    ld de,DOS_SERVICE_END
    call cmp_hl_de
    jp c,mdDosService
    jp z,mdDosService
    ; Весь слот закреплён за одним модулем, включая его статические буферы.
    ; Стек остаётся служебным стеком DOS и в эту арену не входит.
    ld de,DOS_BANK_DRIVER_END
    call cmp_hl_de
    jp c,mdDosEndReady
    jp z,mdDosEndReady
    ld de,DOS_BANK_MODULE_END
    call cmp_hl_de
    jp c,mdDosBorrowBat
    jp nz,mdRange
mdDosBorrowBat:
    ld a,(v_batDepth)
    or a
    jp nz,mdArenaBusy
    ld a,1
    ld (mdBorrowRequest),a
mdDosEndReady:
    ld hl,(mdAddress)
    ld de,DOS_BANK_DRIVER_ADDR
    call cmp_hl_de
    jp c,mdRange
    ld a,(diActive)
    or a
    jp nz,mdArenaBusy
    ld a,(moduleDosOwner)
    or a
    jp nz,mdArenaBusy
    ld hl,(mdRecord)
    ld de,9
    add hl,de
    ld a,(hl)
    ld (moduleDosOwner),a
    ld a,(mdBorrowRequest)
    ld (moduleDosBorrowed),a
    jp mdDosReserved
mdDosService:
    ld hl,(mdAddress)
    ld de,DOS_SERVICE_ADDR
    call cmp_hl_de
    jp c,mdRange
    ld a,(moduleDosServiceOwner)
    or a
    jp nz,mdArenaBusy
    ld hl,(mdRecord)
    ld de,9
    add hl,de
    ld a,(hl)
    ld (moduleDosServiceOwner),a
mdDosReserved:
    ld hl,0
    ld (memorySlotPtr),hl
    ld b,DOS_IO_PAGE
    jp mdReservedRecord
mdArenaBusy:
    ld a,MEM_ERROR_ACTIVE
    jp mdError
mdMain:
    ; Сужаем начало доверенной арены только на время точного выделения.
    ld hl,(memoryMainStart)
    ex de,hl
    ld hl,(mdAddress)
    call cmp_hl_de
    jp c,mdRange
    push de
    ld (memoryMainStart),hl
    ld hl,(mdBytes)
    ld b,0
    call memoryAlloc
    pop de
    ex de,hl
    ld (memoryMainStart),hl
    ex de,hl
    jp c,mdError
    MD_GET_DE mdAddress
    call cmp_hl_de
    jp z,mdReserved
    ld hl,(memorySlotPtr)
    ld (hl),0FFh
    jp mdRange
mdReserved:
    ld a,b
    ld (mdBankId),a
    ld hl,(memorySlotPtr)
    ld de,5
    add hl,de
    ld (hl),0FFh ; Прямой FREE приложения не освобождает модуль.
mdReservedRecord:
    ld a,b
    ld (mdBankId),a
    ld hl,(mdRecord)
    inc hl
    inc hl
    xor a
    ld (hl),a
    inc hl
    ld (hl),b
    inc hl
    MD_GET_DE mdEntry
    ld (hl),e
    inc hl
    ld (hl),d
    inc hl
    MD_GET_DE memorySlotPtr
    ld (hl),e
    inc hl
    ld (hl),d
    ld a,(mdBankId)
    ld (kdPage),a
    cp DOS_IO_PAGE
    jp z,mdContextReady
    or a
    call nz,mdInstall
    jp c,mdRollback
mdContextReady:
    ld hl,(mdAddress)
    ld (kdFarCurrent),hl
    ld hl,0
    ld (kiSum),hl
    ld a,1
    ld (kiSumActive),a
    ld (kdFarActive),a
    ld de,KD_DATA
    call imageLoad
    push af
    xor a
    ld (kiSumActive),a
    ld (kdFarActive),a
    ld a,(mdCaller)
    ld (kdPage),a
    pop af
    jp c,mdRollback
    ld hl,(mdRecord)
    MD_GET_DE mdNext
    ld (hl),e
    inc hl
    ld (hl),d
    ex de,hl
    ld (kdRegs),hl
    inc hl
    ld (mdNext),hl
    xor a
    jp kdError
mdRollback:
    push af
    ld a,(mdCaller)
    ld (kdPage),a
    ld hl,(mdRecord)
    call mdRelease
    pop af
    jp mdError
mdFileError:
    call dosGetError
    jp mdError
mdImage:
    ld a,DOS_ERR_BAD_IMAGE
    jp mdError
mdRange:
    ld a,MEM_ERROR_RANGE
mdError:
    scf
    jp kdError

; Полный профиль берётся из DOS. Пакет физического копировщика внутренний:
; разрешены защищённые хвосты выше FF00h, недоступные публичному memcpy_far.
mdInstall:
    ld hl,MEM_IMAGE_LOCAL_START
    ld de,MEM_IMAGE_LOCAL_START
    ld bc,BANK_LOCAL_BYTES
    call mdInstallPart
    ret c
    ld hl,mx_call
    ld de,mx_call
    ld bc,BANK_CLIENT_BYTES
    call mdInstallPart
    ret c
    ld hl,BANK_LANDING
    ld de,BANK_LANDING
    ld bc,BANK_TRANSPORT_BYTES
    call mdInstallPart
    ret c
    ld a,(mdBankId)
    ld (mdContext),a
    ld a,(bankPageCount)
    ld (mdContext+1),a
    ld hl,mdContext
    ld de,bankPage
    ld bc,9
    call mdInstallPart
    ret c
    ld hl,mdLocalContext
    ld de,MEM_NATIVE_CALL
    ld bc,5
    call mdInstallPart
    ret c
    ld hl,mdLocalContext
    ld de,MEM_NEAR_COPY
    ld bc,2
mdInstallPart:
    ld (mdCopy+1),hl
    MD_PUT_DE mdCopy+4
    MD_PUT_BC mdCopy+6
    ld a,(mdBankId)
    ld (mdCopy+3),a
    ld hl,mdCopy
    jp bankCopySector

; Проверка opaque ID и владельца. HL=запись, CF=0; AF/BC/DE разрушены.
mdResolve:
    MD_GET_DE kdRegs
    ld a,d
    or e
    jp z,mdHandle
    ld hl,MODULE_TABLE
    ld b,MODULE_MAX
mdFind:
    ld a,(hl)
    cp e
    inc hl
    jp nz,mdFindNext
    ld a,(hl)
    cp d
    jp z,mdFound
mdFindNext:
    ld a,MODULE_RECORD_SIZE-1
    add a,l
    ld l,a
    jp nc,mdFindCarry
    inc h
mdFindCarry:
    dec b
    jp nz,mdFind
mdHandle:
    ld a,MEM_ERROR_HANDLE
    scf
    ret
mdFound:
    dec hl
    call residentKey
    inc hl
    push af
    inc hl
    ld a,(hl)
    or a
    jp nz,mdFoundActive
    pop af
    jp nz,mdResidentFound
    ld de,6
    add hl,de
    call memoryOwnerCurrent
    cp (hl)
    jp nz,mdDenied
    ld de,-6
    add hl,de
    dec hl
    dec hl
    xor a
    ret
mdFoundActive:
    pop af
    jp mdActive
mdResidentFound:
    dec hl
    dec hl
    xor a
    ret
mdDenied:
    ld a,MEM_ERROR_OWNER
    scf
    ret
mdActive:
    ld a,MEM_ERROR_ACTIVE
    scf
    ret

moduleUnload:
    call mdResolve
    jp c,mdError
    ld a,(kdRegs+7)
    cp 1
    jp nz,mdExplicitUnload
    call residentKey
    jp z,mdExplicitUnload
    xor a
    jp kdError
mdExplicitUnload:
    push hl
    ld de,9
    add hl,de
    ld a,(memoryRamOwner)
    or a
    jp z,mdUnloadAllowed
    cp (hl)
    jp z,mdUnloadPinned
mdUnloadAllowed:
    pop hl
    call mdRelease
    jp c,mdError
    xor a
    jp kdError
mdUnloadPinned:
    pop hl
    ld a,MEM_ERROR_ACTIVE
    jp mdError

; HL=запись. Потомки освобождаются до владельца, реестр конечен и ацикличен.
mdRelease:
    push hl
    call klReclaim
    pop hl
    ret c
    call residentKey
    xor a
    ld (de),a
    inc de
    ld (de),a
    push hl
    ld de,9
    add hl,de
    ld c,(hl)
    ld hl,MODULE_TABLE
    ld b,MODULE_MAX
mdChildren:
    push hl
    ld de,8
    add hl,de
    ld a,(hl)
    cp c
    pop hl
    jp nz,mdChildNext
    ld a,(hl)
    inc hl
    or (hl)
    dec hl
    jp z,mdChildNext
    push bc
    push hl
    call mdRelease
    pop hl
    pop bc
mdChildNext:
    ld de,MODULE_RECORD_SIZE
    add hl,de
    dec b
    jp nz,mdChildren
    ld hl,MEM_ALLOC_TABLE
    ld b,MEM_ALLOC_RECORDS
mdOwned:
    push hl
    ld de,5
    add hl,de
    ld a,(hl)
    cp c
    pop hl
    jp nz,mdOwnedNext
    ld (hl),0FFh
mdOwnedNext:
    ld de,6
    add hl,de
    dec b
    jp nz,mdOwned
    ld a,(memoryMainOwner)
    cp c
    jp nz,mdFreeImage
    ld hl,0
    ld (memoryMainStart),hl
    ld (memoryMainEnd),hl
mdFreeImage:
    pop hl
    push hl
    ld de,6
    add hl,de
    ld e,(hl)
    inc hl
    ld d,(hl)
    ld a,d
    or e
    jp z,mdFreeRecord
    ex de,hl
    ld (hl),0FFh
mdFreeRecord:
    pop hl
    push hl
    ld de,3
    add hl,de
    ld a,(hl)
    or a
    jp z,mdFreeRecordMain
    cp DOS_IO_PAGE
    jp nz,mdFreeExec
    inc hl
    inc hl                 ; Старший байт входа различает два слота DOS.
    ld a,(hl)
    cp DOS_SERVICE_END/256
    jp nc,mdFreeTransientDos
    xor a
    ld (moduleDosServiceOwner),a
    jp mdFreeRecordMain
mdFreeTransientDos:
    xor a
    ld (moduleDosOwner),a
    ld (moduleDosBorrowed),a
    jp mdFreeRecordMain
mdFreeExec:
    ld (mdBankId),a
    ld hl,mdDataKind
    ld de,MEM_BANK_KIND
    ld bc,1
    call mdInstallPart
mdFreeRecordMain:
    pop hl
    xor a
    ld (hl),a
    inc hl
    ld (hl),a
    ret

; HL=handle, DE=MX_REGS в банке инициатора. Модуль получает регистры,
; обычный RET возвращает их в пакет; AF самого API — статус диспетчеризации.
moduleCall:
    call mdResolve
    jp c,mdError
    ld (mdRecord),hl
    ld hl,(kdRegs+2)
    ld (mdClientRegs),hl
    ; Результат не может затереть локальный хвост MEM или аппаратное окно.
    push hl
    ld bc,MX_REGS
    add hl,bc
    ld de,MEM_IMAGE_LOCAL_START
    call cmp_hl_de
    pop hl
    jp c,mdRegsReady
    jp nz,kdBad
mdRegsReady:
    ld de,KD_OBJECT
    ld bc,MX_REGS
    call kdFetch
    jp c,kdBad
    ld a,(moduleActor)
    push af
    ld hl,(mdRecord)
    push hl
    inc hl
    inc hl
    inc (hl)
    inc hl
    ld a,(hl)
    ld (mdBankId),a
    inc hl
    ld e,(hl)
    inc hl
    ld d,(hl)
    MD_PUT_DE mdCallEntry
    ld de,4
    add hl,de
    ld a,(hl)
    ld (moduleActor),a
    ld a,(kdPage)
    push af
    ld hl,(mdClientRegs)
    push hl
    ld hl,(KD_OBJECT+6)
    push hl
    ld hl,(KD_OBJECT+4)
    push hl
    ld hl,(KD_OBJECT+2)
    push hl
    ld hl,(KD_OBJECT)
    push hl
    ld hl,0
    add hl,sp
    ex de,hl
    xor a
    ld (kdBusy),a
    MD_GET_BC mdCallEntry
    ld a,(mdBankId)
    ld hl,bankModuleCallback
    call bankCallPacket
    push af
    push bc
    push de
    push hl
    ; Состояние внешней операции лежит в стеке, вложенные DOS-вызовы
    ; могут использовать все глобальные временные kd*/md* заново.
    ld hl,19
    add hl,sp
    ld a,(hl)
    ld (kdPage),a
    inc hl
    ld e,(hl)
    inc hl
    ld d,(hl)
    inc de
    inc de
    xor a
    ld (de),a
    inc hl
    inc hl
    ld a,(hl)
    ld (moduleActor),a
    ld a,(BANK_FRAME_STATUS)
    or a
    jp nz,mdCallRejected
    ld hl,16
    add hl,sp
    ld e,(hl)
    inc hl
    ld d,(hl)
    ld hl,0
    add hl,sp
    ld bc,MX_REGS
    call kdStore
    jp mdCallDone
mdCallRejected:
    ld hl,6
    add hl,sp
    ld e,(hl)
    inc hl
    ld d,(hl)
    push de
    pop af
mdCallDone:
    push af
    pop bc
    ld hl,24
    add hl,sp
    ld sp,hl
    push bc
    pop af
    jp c,mdError
    xor a
    jp kdError

moduleActor: DB 0
moduleDosBorrowed: DB 0 ; DOS-DLL занимает неактивные кадры BAT.
mdBorrowRequest: DB 0
moduleDosOwner: DB 0 ; Временная арена DRIVER; не выделение FAR-кучи.
moduleDosServiceOwner: DB 0 ; Отдельный резидентный слот DOS.
mdNext: DW 1
mdRecord: DW 0
mdClientRegs: DW 0
mdCaller: DB 0
mdOwner: DB 0
mdPlacement: DB 0
mdBankId: DB 0
mdAddress: DW 0
mdEntry: DW 0
mdBytes: DW 0
mdCallEntry: DW 0
mdCopy: DB 1
    DW 0
    DB 0
    DW 0,0
mdContext: DB 0,16,0,0,0
    DW DOS_BANK_STACK_TOP,DOS_BANK_STACK_BOTTOM+64
mdLocalContext: DW 0
    DB MEM_BANK_EXEC
    DW 0
mdDataKind: DB MEM_BANK_DATA

    INCLUDE "lifecycle.inc"
