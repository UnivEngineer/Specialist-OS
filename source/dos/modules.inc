; Модули v2/v3. Фактические адреса и владение хранит DOS.
; Публичный вызов через MXOS_CALL; все NEAR-параметры находятся у kdPage.
; Запись: handle:word, active:byte, bank:byte, entry:word,
; allocation:word, owner:byte, allocationOwner:byte, start:word, end:word,
; options:byte, policy:byte. With handle=0, active=1 marks a vacant image.
; Диапазоны полуоткрытые; ID не переиспользуются.
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
    ld (fileopsReady),a
    ld (enumReady),a
    ld (fileopsActive),a
    ld (enumActive),a
    ld (fileopsPublic),a
    ld (moduleActor),a
    ld (appOwner),a
    ld (appDepth),a
    ld hl,0
    ld (appImage),hl
    ld (keImageToken),hl
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

;----------------------------------------------------------------------------
; MXOS
;
; Определить владельца выделений памяти для текущего вызова DOS.
;
; вход:
;   moduleActor, appOwner и kdPage описывают текущий контекст. Регистровых аргументов
;   нет.
;
; выход:
;   a = первый ненулевой владелец из moduleActor и appOwner, иначе исходная страница
;   kdPage. bc, de и hl сохранены. Флаги не задают единый статус.
;----------------------------------------------------------------------------

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
; FILEOPS code stays cached; a lost caller cannot resume its old job/stack.
moduleWarm:
    ld a,MEM_EXEC_READY_VALUE
    ld (MEM_EXEC_READY),a
    xor a
    ld (timerOwner),a
    ld (fileopsActive),a
    ld (enumActive),a
    ld (fileopsPublic),a
    ld (moduleActor),a
    ld (kiSumActive),a
    ; Reset во время установки EXEC теряет стек и неопубликованный handle.
    ; Сначала возвращаем профиль DATA, потом обычный проход освободит резерв.
    ld a,(mdLoadingBank)
    or a
    jp z,mdWarmRoutes
    ld hl,(mdRecord)
    ld a,(hl)
    inc hl
    or (hl)
    ld a,(mdLoadingBank)
    call z,mdRestoreData
    xor a
    ld (mdLoadingBank),a
mdWarmRoutes:
    xor a
    call mdForgetRoute
    ld a,DOS_BANK_PAGE
    call mdForgetRoute
    ld hl,MODULE_TABLE+2
    ld b,MODULE_MAX
mdResetActive:
    push bc
    push hl
    dec hl
    ld a,(hl)
    dec hl
    or (hl)
    jp z,mdResetRouteDone
    inc hl
    inc hl
    inc hl
    ld a,(hl)
    cp 2
    call nc,mdRefreshExec
mdResetRouteDone:
    pop hl
    pop bc
    xor a
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
    ; После потери стеков MAIN-буферы клиентов регистрируются заново.
    ; Сохраняем системные блоки и диапазоны опубликованных образов DLL.
    ld hl,MEM_ALLOC_TABLE
    ld b,MEM_ALLOC_RECORDS
mdWarmBlocks:
    ld a,(hl)
    inc a
    jp z,mdWarmBlockNext
    push hl
    ld de,5
    add hl,de
    ld a,(hl)
    pop hl
    cp DOS_IO_PAGE
    jp z,mdWarmBlockNext
    cp 0FFh
    jp nz,mdWarmOrdinary
    call mdAllocationPublished
    jp nz,mdWarmBlockNext
    jp mdWarmFree
mdWarmOrdinary:
    cp MODULE_OWNER_BASE
    jp c,mdWarmFree
    ld a,(hl)
    or a
    jp nz,mdWarmBlockNext
mdWarmFree:
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
    ld (appImage),hl
    ld (keImageToken),hl
    ld de,APP_MEMORY_END
    jp mdReuseInvalidate

; NZ if a private image reservation belongs to a published module. A Reset
; during preparation must release otherwise orphaned FF-owned allocations.
; DE is scratch.
mdAllocationPublished:
    push bc
    push hl
    ex de,hl
    ld hl,MODULE_TABLE
    ld b,MODULE_MAX
mdAllocationNext:
    ld a,(hl)
    inc hl
    or (hl)
    dec hl
    jp z,mdAllocationSkip
    push hl
    push de
    ld de,6
    add hl,de
    ld c,(hl)
    inc hl
    ld a,(hl)
    ld h,a
    ld l,c
    pop de
    call cmp_hl_de
    pop hl
    jp z,mdAllocationLive
mdAllocationSkip:
    push de
    ld de,MODULE_RECORD_SIZE
    add hl,de
    pop de
    dec b
    jp nz,mdAllocationNext
    xor a
    jp mdAllocationDone
mdAllocationLive:
    ld a,1
    or a
mdAllocationDone:
    pop hl
    pop bc
    ret

;----------------------------------------------------------------------------
; MXOS
; Загрузить модуль в выбранную область и выдать handle после проверки образа.
; вход: kdRegs.HL = путь в kdPage, kdRegs.A = MAIN/BANK/DOS. Таблицы DOS
; инициализированы, страница и владелец определяют контекст клиента.
; выход: CF=0, HL=handle. CF=1, A=ошибка; частный резерв снят, управление
; возвращено на страницу клиента. BC/DE рабочие. База живёт до выгрузки.
;----------------------------------------------------------------------------
moduleLoad:
    xor a
    call dosClearError
    ld a,(kdRegs+7)
    cp MODULE_TARGET_DOS+1
    jp nc,mdRange
    ld (mdPlacement),a
    cp MODULE_TARGET_BANK
    jp z,mdResidentPlacementReady
    ; A retained EXEC may use a tracked, disposable MAIN graphics module.
    ; It remains a child; retaining a module initially still requires no MAIN.
    or a
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
    ld hl,(kdRegs)
    call kdFetchPath
    jp c,kdBad
    ld hl,KD_PATH
    ld de,v_curFileDescr
    call fileNamePrepare
    jp c,mdFileError
    ld hl,v_curFileDescr
    call imageOpenModule
    jp c,mdFileError
    ld hl,(v_imageHeader+IMAGE_STACK)
    ld de,DOS_BANK_STACK_TOP-DOS_BANK_STACK_BOTTOM-64
    ld a,(mdPlacement)
    or a
    jp nz,mdStackLimit
    ld de,BIOS_BANK_STACK_TOP-BIOS_BANK_STACK_BOTTOM-64
mdStackLimit:
    call cmp_hl_de
    jp c,mdStackReady
    jp nz,mdRange
mdStackReady:
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
    ld a,(v_imageHeader+IMAGE_OPTIONS)
    and IMAGE_OPTION_REUSABLE
    jp z,mdPlacementDispatch
    ld a,(mdPlacement)
    or a
    jp nz,mdImage
mdPlacementDispatch:
    call mdReuseSelect
    jp c,mdNoMemory
    ld (mdRecord),hl
    push bc
    ld a,(mdPlacement)
    call imagePlace
    pop bc
    jp c,mdError
    ld (mdEntry),hl
    ex de,hl
    ld (mdAddress),hl
    ld hl,(v_imageHeader+IMAGE_MEMORY_BEGIN)
    ld (mdMemoryStart),hl
    ld hl,(v_imageHeader+IMAGE_MEMORY_END)
    ld (mdMemoryEnd),hl
    ld hl,(mdRecord)
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
    ld hl,(mdMemoryEnd)
    ld a,(mdPlacement)
    cp MODULE_TARGET_DOS
    jp z,mdDos
    or a
    jp z,mdMain
    push hl
    ld hl,(v_imageHeader+IMAGE_MIN_HEAP)
    ex de,hl
    pop hl
    add hl,de
    ld de,DOS_BANK_STACK_BOTTOM
    call cmp_hl_de
    jp c,mdBank
    jp nz,mdRange
mdBank:
    ; Весь DATA-банк резервируется целиком: чужие блоки исключены.
    ld hl,MEM_DATA_LIMIT
    ld b,MEM_ALLOC_ANY
    call memoryAlloc
    jp c,mdError
    jp mdReserved
mdDos:
    push hl
    ld hl,(v_imageHeader+IMAGE_BSS)
    inc hl
    ld a,h
    or l
    pop hl
    jp z,mdRange
    ; One contiguous arena, shared by cached services and installers.
    ld de,DOS_BANK_MODULE_END
    call cmp_hl_de
    jp c,mdDosEndReady
    jp nz,mdRange
mdDosEndReady:
    ld hl,(mdMemoryStart)
    ld de,DOS_ARENA_START
    call cmp_hl_de
    jp c,mdRange
    MD_GET_DE mdMemoryEnd
    call imageClaimDos
    jp c,mdArenaBusy
    ld hl,0
    ld (memorySlotPtr),hl
    ld b,DOS_IO_PAGE
    jp mdReservedRecord
mdArenaBusy:
    ld a,MEM_ERROR_ACTIVE
    jp mdError
mdMain:
    call memoryFindFreeSlot
    jp c,mdError
    ld hl,(mdMemoryStart)
    MD_GET_DE mdMemoryEnd
    ld bc,APP_MEMORY_END
    push hl
    ld h,b
    ld l,c
    call cmp_hl_de
    pop hl
    jp c,mdRange
    MD_GET_BC mdMemoryEnd
    call moduleMainClaimStrict
    jp c,mdError
    ld hl,(mdMemoryStart)
    MD_GET_DE mdMemoryEnd
    call memoryReserveMain
    jp c,mdError
    ld b,0
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
    inc hl
    inc hl
    inc hl                  ; record+10, after the two owner bytes.
    MD_GET_DE mdMemoryStart
    ld (hl),e
    inc hl
    ld (hl),d
    inc hl
    MD_GET_DE mdMemoryEnd
    ld (hl),e
    inc hl
    ld (hl),d
    inc hl
    ld a,(v_imageHeader+IMAGE_OPTIONS)
    ld (hl),a
    inc hl
    ld a,(v_imageHeader+IMAGE_POLICY)
    ld (hl),a
    ld a,(mdBankId)
    ld (kdPage),a
    cp DOS_IO_PAGE
    jp z,mdContextReady
    or a
    jp z,mdContextReady
    ld (mdLoadingBank),a
    call mdInstall
    jp c,mdRollback
mdContextReady:
    ld a,(mdReuseHit)
    or a
    jp z,mdReadImage
    ld c,1
    call mdActivate
    jp nc,mdLoadReady
mdReadImage:
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
    pop af
    jp c,mdRollback
    ld a,(v_imageHeader+IMAGE_OPTIONS)
    and IMAGE_OPTION_REUSABLE
    jp z,mdLoadReady
    ld c,0
    call mdActivate
    jp c,mdRollback
mdLoadReady:
    ld a,(mdCaller)
    ld (kdPage),a
    ld hl,(mdNext)
    ld (kdRegs),hl
    ld d,h
    ld e,l
    inc hl
    ld (mdNext),hl
    ld hl,(mdRecord)
    ld (hl),e
    inc hl
    ld (hl),d
    xor a
    ld (mdLoadingBank),a
    call mdReuseTrack
    call memoryPublishTop
    xor a
    jp kdError
mdRollback:
    push af
    ld a,(mdCaller)
    ld (kdPage),a
    ld hl,(mdRecord)
    ; Handle ещё не выдан. У этой записи нет потомков или захваченных служб.
    call mdDiscardImage
    xor a
    ld (mdLoadingBank),a
    pop af
    jp mdError
mdFileError:
    call dosGetError
    jp mdError
mdImage:
    ld a,DOS_ERR_BAD_IMAGE
    jp mdError
mdNoMemory:
    ld a,MEM_ERROR_NOMEM
    jp mdError
mdRange:
    ld a,MEM_ERROR_RANGE
mdError:
    scf
    jp kdError

; Полный профиль берётся из DOS. Пакет физического копировщика внутренний:
; разрешены защищённые хвосты выше FF00h, недоступные публичному memcpy_far.
mdForgetRoute:
    ld (mdBankId),a
    ld hl,bankQueueResetImage
    ld de,MEM_ROUTE_STATE
    ld bc,bankQueueBefore-MEM_ROUTE_STATE+1
    jp mdInstallPart
mdRefreshExec:
    call mdForgetRoute
    ret c
mdMarkExec:
    ld hl,mdExecReady
    ld de,MEM_EXEC_READY
    ld bc,1
    jp mdInstallPart
mdInstall:
    ld hl,IO_BANK_KEYS
    ld de,IO_BANK_KEYS
    ld bc,MEM_IMAGE_LOCAL_START-IO_BANK_KEYS+BANK_LOCAL_BYTES
    call mdInstallPart
    ret c
    ld hl,mx_call
    ld de,mx_call
    ld bc,BANK_CLIENT_BYTES
    call mdInstallPart
    ret c
    ld hl,BANK_LANDING
    ld de,BANK_LANDING
    ; Хвост DATA уже установлен загрузчиком. Его счётчик и селекторы
    ; обслуживают этот самый перенос и должны дожить до возврата в DOS.
    ld bc,MEM_DATA_START-BANK_LANDING
    call mdInstallPart
    ret c
    ; Сначала вернуть подготовку прямого MOVE, затем открыть её вход.
    ld hl,bankMoveDataEntry
    ld de,bankMoveDataEntry
    ld bc,bankMoveDataBytes
    call mdInstallPart
    ret c
    ld hl,bankMoveDirectOperand
    ld de,bankMoveDirectOperand
    ld bc,2
    call mdInstallPart
    ret c
    ld a,(mdBankId)
    ld (mdContext),a
    ld a,(bankPageCount)
    ld (mdContext+1),a
    ld a,(mdBankId)
    call mdForgetRoute
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
    ld hl,mdNearCopy
    ld de,MEM_NEAR_COPY
    ld bc,2
    call mdInstallPart
    ret c
    jp mdMarkExec
mdInstallPart:
    ld (mdCopy+1),hl
    MD_PUT_DE mdCopy+4
    MD_PUT_BC mdCopy+6
    ld a,(mdBankId)
    ld (mdCopy+3),a
    ld hl,mdCopy
    jp bankCopySector

;----------------------------------------------------------------------------
; MXOS
;
; Найти запись модуля по дескриптору запроса и проверить доступ к ней.
;
; вход:
;   Первое слово kdRegs содержит непрозрачный дескриптор. Для частного модуля
;   проверяется владелец текущего вызова.
;
; выход:
;   cf = 0: hl указывает на запись MODULE_TABLE. cf = 1: a = код ошибки дескриптора,
;   владельца или активного модуля. bc и de используются для поиска.
;----------------------------------------------------------------------------

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

; Preflight the whole ownership subtree. An inactive cached parent must not
; evict an executing child. Iterate a bounded owner set, without recursive SP.
mdCanRelease:
    push bc
    push de
    push hl
    push hl
    inc hl
    inc hl
    ld a,(hl)
    or a
    jp nz,mdGuardRootActive
    pop hl
    ld de,9
    add hl,de
    ld a,(hl)
    sub MODULE_OWNER_BASE
    cp MODULE_MAX
    jp nc,mdGuardInvalid
    push af
    ld hl,DOS_MODULE_GUARD_ADDR
    ld b,MODULE_MAX
    xor a
mdGuardClear:
    ld (hl),a
    inc hl
    dec b
    jp nz,mdGuardClear
    pop af
    ld l,a
    ld h,0
    ld de,DOS_MODULE_GUARD_ADDR
    add hl,de
    ld (hl),1
mdGuardPass:
    xor a
    ld (DOS_MODULE_GUARD_ADDR+MODULE_MAX),a
    ld hl,MODULE_TABLE
    ld b,MODULE_MAX
mdGuardRow:
    ld a,(hl)
    inc hl
    or (hl)
    dec hl
    jp z,mdGuardNext
    push hl
    ld de,8
    add hl,de
    ld a,(hl)
    sub MODULE_OWNER_BASE
    cp MODULE_MAX
    jp nc,mdGuardPop
    ld l,a
    ld h,0
    ld de,DOS_MODULE_GUARD_ADDR
    add hl,de
    ld a,(hl)
    or a
    jp z,mdGuardPop
    pop hl
    push hl
    inc hl
    inc hl
    ld a,(hl)
    or a
    jp nz,mdGuardRootActive
    ld de,7
    add hl,de
    ld a,(hl)
    sub MODULE_OWNER_BASE
    cp MODULE_MAX
    jp nc,mdGuardPop
    ld l,a
    ld h,0
    ld de,DOS_MODULE_GUARD_ADDR
    add hl,de
    ld a,(hl)
    or a
    jp nz,mdGuardPop
    inc a
    ld (hl),a
    ld (DOS_MODULE_GUARD_ADDR+MODULE_MAX),a
mdGuardPop:
    pop hl
mdGuardNext:
    ld de,MODULE_RECORD_SIZE
    add hl,de
    dec b
    jp nz,mdGuardRow
    ld a,(DOS_MODULE_GUARD_ADDR+MODULE_MAX)
    or a
    jp nz,mdGuardPass
    xor a
mdGuardDone:
    pop hl
    pop de
    pop bc
    ret
mdGuardRootActive:
    pop hl
    ld a,MEM_ERROR_ACTIVE
    scf
    jp mdGuardDone
mdGuardInvalid:
    ld a,MEM_ERROR_HANDLE
    scf
    jp mdGuardDone

; HL=запись. Потомки освобождаются до владельца, реестр конечен и ацикличен.
mdRelease:
    call mdCanRelease
    ret c
mdReleaseChecked:
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
    call timerReleaseOwner
    call fileopsReleaseOwner
    call enumReleaseOwner
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
    call mdReleaseChecked
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
mdDiscardImage:
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
    jp mdFreeRecordMain
mdFreeExec:
    call mdRestoreData
    jp mdFreeRecordMain

;----------------------------------------------------------------------------
; MXOS
; Вернуть хвост освобождаемой страницы из EXEC в DATA, в том числе после Reset.
; вход: A=выделенная физическая FAR-страница, транспорт DOS уже работает.
; выход: профиль DATA установлен, AF/BC/DE/HL рабочие, mdBankId изменён.
;----------------------------------------------------------------------------
mdRestoreData:
    ld (mdBankId),a
    ; Вход DATA включается после записи всего тела прямого MOVE.
    ld hl,mdDataForwardTemplate
    ld de,bankMoveDataEntry
    ld bc,bankMoveDataBytes
    call mdInstallPart
    ld hl,mdDataForwardWord
    ld de,bankMoveDirectOperand
    ld bc,2
    call mdInstallPart
    ld hl,mdDataKind
    ld de,MEM_EXEC_READY
    ld bc,1
    call mdInstallPart
    ld hl,mdDataKind
    ld de,MEM_BANK_KIND
    ld bc,1
    jp mdInstallPart
mdFreeRecordMain:
    pop hl
    xor a
    ld c,a
    or (hl)
    inc hl
    or (hl)
    jp z,mdFreeRecordClear
    push hl
    ld de,13
    add hl,de
    ld a,(hl)
    and IMAGE_OPTION_REUSABLE
    rrca
    ld c,a
    pop hl
mdFreeRecordClear:
    xor a
    ld (hl),a
    dec hl
    ld (hl),a
    inc hl
    inc hl
    ld (hl),c
    call memoryPublishTop
    ret

; Resolve and pin once for the duration of the controller callback.
moduleScope:
    call mdResolve
    jp c,mdError
    push hl
    inc hl
    inc hl
    inc (hl)
    inc hl
    ld a,(hl)
    ld (kdRegs+4),a
    inc hl
    ld e,(hl)
    inc hl
    ld d,(hl)
    ld bc,4
    add hl,bc
    ld a,(hl)
    ld (kdRegs+5),a
    ld hl,(kdRegs+2)
    ld b,h
    ld c,l
    MD_PUT_DE kdRegs+2
    xor a
    ld (kdBusy),a
    ld a,(kdPage)
    ld hl,MEM_SCOPE_ENTRY
    ld de,kdRegs
    call bankCallPacket
    pop hl
    inc hl
    inc hl
    dec (hl)
    xor a
    jp kdError
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
    ld de,IO_BANK_KEYS
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

    INCLUDE "module-main-claim.inc"
    INCLUDE "module-reuse.inc"
    INCLUDE "module-relocate.inc"

    INCLUDE "lifecycle.inc"

mdDataForwardWord: DW bankMoveDataEntry
mdDataForwardTemplate:
    DISP bankMoveDataEntry
    INCLUDE "../mem/data-forward.inc"
    ENT
    ASSERT $-mdDataForwardTemplate == bankMoveDataBytes
