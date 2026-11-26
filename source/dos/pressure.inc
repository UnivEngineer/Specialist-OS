;----------------------------------------------------------------------------
; MXOS
; Индекс и таблицы политики банковых модулей.
; вход: HL=запись MODULE_TABLE (pressureSlot), A=индекс (остальные входы).
; выход: A=индекс, либо HL=адрес элемента. AF, DE рабочие.
;----------------------------------------------------------------------------
pressureSlot:
    ld a,l
    sub MODULE_TABLE & 255
    rrca
    rrca
    rrca
    rrca
    ret
pressureByte:
    ld e,a
    ld d,0
    add hl,de
    ret
pressurePath:
    ld l,a
    ld h,0
    DUP 5
        add hl,hl
    EDUP
    ld de,pressurePaths
    add hl,de
    ret
;----------------------------------------------------------------------------
; MXOS
; Инициализировать приоритет либо освободить политику.
; вход: mdRecord, KD_PATH при загрузке, HL=запись при освобождении.
; выход: AF/BC/DE/HL рабочие. Ожидающий повторной загрузки путь сохраняется.
;----------------------------------------------------------------------------
pressureInitModule:
    ld hl,(mdRecord)
    call pressureSlot
    ld hl,residentPriorities
    call pressureByte
    ld (hl),RESIDENT_PRIORITY_APP
    ret
pressureSlotCount = 2

pressureStringCopy:
    ld a,(hl)
    ld (de),a
    or a
    ret z
    inc hl
    inc de
    dec b
    jp nz,pressureStringCopy
    xor a
    ld (de),a
    ret
pressureForgetModule:
    call pressureSlot
    ld hl,residentManaged
    call pressureByte
    ld (hl),0
    ld hl,residentVetoCodes
    call pressureByte
    ld (hl),0
    ld hl,residentReloadSlots
    call pressureByte
    ld a,(hl)
    ld (hl),0
    or a
    ret z
    dec a
    ld hl,residentReloadStatus
    call pressureByte
    ld a,(hl)
    cp 2
    ret z
    ld (hl),0
    ret

;----------------------------------------------------------------------------
; MXOS
; Исполнить команду после снятия адреса возврата из банка NC.
; вход: установленный запрос RESIDENT_EXEC, стек диспетчера DOS.
; выход: A=0, CF=0, kdPage восстановлена. Как при возврате в NC, ошибка
; дочерней команды не переключает предпочтительную оболочку. BC/DE/HL рабочие.
;----------------------------------------------------------------------------
pressureRunCommand:
    ld a,(pressureCommandReady)
    or a
    ret z
    xor a
    ld (pressureCommandReady),a
    ld a,(kdPage)
    push af
    ld hl,CMD_LINE
    call fileExec
    pop bc
    ld a,b
    ld (kdPage),a
    xor a
    ret

;----------------------------------------------------------------------------
; MXOS
; Выдать банк, повторяя исходный ALLOC после каждого успешного вытеснения.
; вход: B=банк или ANY, HL=размер. MAIN и ошибочные размеры идут обычным путём.
; выход: как ALLOC; при отказе E8h, B:C=банк строк и код, HL/DE=имя/причина.
;----------------------------------------------------------------------------
pressureAlloc:
    ld a,b
    or a
    jp z,memoryAlloc
    cp MEM_ALLOC_ANY
    jp z,pressureAllocSize
    cp 2
    jp c,memoryAlloc
    ld a,(bankPageCount)
    cp b
    jp c,memoryAlloc
pressureAllocSize:
    ld de,MEM_DATA_LIMIT
    call cmp_hl_de
    jp c,pressureAllocTry
    jp nz,memoryNoMem
pressureAllocTry:
    push bc
    push hl
    call memoryAlloc
    jp nc,pressureAllocated
    pop hl
    pop bc
    push bc
    push hl
    call pressureReclaim
    jp nc,pressureRetry
pressureAllocated:
    inc sp
    inc sp
    inc sp
    inc sp
    ret
pressureRetry:
    pop hl
    pop bc
    jp pressureAllocTry

;----------------------------------------------------------------------------
; MXOS
; Зарегистрировать политику, отказ, команду или снимок текущей оболочки.
; вход: kdRegs и kdPage клиента. POLICY и VETO относятся к текущему модулю.
; выход: CF/A передаются диспетчеру. Путь автовосстановления копируется в DOS,
; указатель причины принадлежит модулю до снятия отказа или выгрузки.
;----------------------------------------------------------------------------
pressureService:
    ld a,(kdRegs+7)
    ld c,a
    ld a,(moduleActor)
    sub MODULE_OWNER_BASE
    cp MODULE_MAX
    jp nc,residentDenied
    ld (pressureIndex),a
    ld a,c
    cp RESIDENT_POLICY
    jp z,pressurePolicy
    cp RESIDENT_VETO
    jp z,pressureVeto
    cp RESIDENT_EXEC
    jp nz,kdBad
    call pressureNcActor
    jp nz,residentDenied
    ld hl,(kdRegs)
    ld de,CMD_LINE
    ld bc,MAX_PATH*256+32
    call kdReadString
    jp c,kdBad
    ld a,1
    ld (pressureCommandReady),a
    jp pressureOK
;----------------------------------------------------------------------------
; MXOS
; Проверить право на сохранение снимка и передачу команды оболочки.
; вход: текущий moduleActor. Выход: Z=1 только при ключе NC. AF/DE/HL рабочие.
;----------------------------------------------------------------------------
pressureNcActor:
    ld a,(moduleActor)
    sub MODULE_OWNER_BASE
    cp MODULE_MAX
    jp nc,pressureNotNc
    add a,a
    ld hl,residentKeys
    call pressureByte
    ld a,(hl)
    cp NC_BANK_KEY & 255
    ret nz
    inc hl
    ld a,(hl)
    cp NC_BANK_KEY >> 8
    ret
pressureNotNc:
    or 1
    ret
pressureVeto:
    ld a,(pressureIndex)
    add a,a
    push af
    ld hl,residentVetoFlags
    call pressureByte
    ex de,hl
    ld hl,(kdRegs+2)
    call appSaveWord
    pop af
    ld hl,residentVetoTexts
    call pressureByte
    ex de,hl
    ld hl,(kdRegs)
    call appSaveWord
    ld a,(pressureIndex)
    ld hl,residentVetoCodes
    call pressureByte
    ld a,(kdRegs+4)
    ld (hl),a
    jp pressureOK
pressurePolicy:
    ld a,(kdRegs+4)
    cp 2
    jp nc,kdBad
    or a
    jp z,pressureSetPriority
    ld hl,(kdRegs)
    ld de,KD_PATH
    ld bc,PRESSURE_PATH_BYTES*256+1
    call kdReadString
    jp c,kdBad
    ld hl,(KD_PATH)
    ld a,h
    cp ':'
    jp nz,kdBad
    ld a,l
    sub 'A'
    cp 8
    jp nc,kdBad
    ld a,(KD_PATH+2)
    cp 05Ch
    jp z,pressurePolicyPath
    cp '/'
    jp nz,kdBad
pressurePolicyPath:
    ld a,(pressureIndex)
    ld hl,residentReloadSlots
    call pressureByte
    ld a,(hl)
    or a
    jp nz,pressureSetPath
    ld a,(pressureBusy)
    or a
    jp z,pressureLookForPath
    ld a,(pressureReloadIndex)
    cp pressureSlotCount
    jp nc,pressureLookForPath
    push hl
    ld hl,residentReloadStatus
    call pressureByte
    ld (hl),1
    pop hl
    inc a
    ld (hl),a
    jp pressureSetPath
pressureLookForPath:
    ld de,residentReloadStatus
    ld b,pressureSlotCount
pressureFreePath:
    ld a,(de)
    or a
    jp z,pressureNewPath
    inc de
    dec b
    jp nz,pressureFreePath
    jp mdNoMemory
pressureNewPath:
    ld a,1
    ld (de),a
    ld a,pressureSlotCount+1
    sub b
    ld (hl),a
pressureSetPath:
    dec a
    call pressurePath
    ex de,hl
    ld hl,KD_PATH
    ld b,PRESSURE_PATH_BYTES-1
    call pressureStringCopy
    jp pressurePublish
pressureSetPriority:
    ld a,(pressureIndex)
    ld hl,residentReloadSlots
    call pressureByte
    ld a,(hl)
    ld (hl),0
    or a
    jp z,pressurePublish
    dec a
    ld hl,residentReloadStatus
    call pressureByte
    ld (hl),0
pressurePublish:
    ld a,(pressureIndex)
    ld hl,residentPriorities
    call pressureByte
    push hl
    ld hl,residentManaged
    call pressureByte
    ld (hl),1
    pop hl
    ld a,(kdRegs+5)
    ld (hl),a
pressureOK:
    xor a
    jp kdError

;----------------------------------------------------------------------------
; MXOS
; Освободить один подходящий EXEC или пустой пул RAM-диска.
; вход: B=банк или FFh. Проверяется всё поддерево владельца.
; выход: CF=0 после освобождения. При исчерпании кандидатов CF=1, A=E3h/E8h.
; При E8h указатели ведут в DOS до следующего запроса с вытеснением.
;----------------------------------------------------------------------------
pressureReclaim:
    ld a,(pressureBusy)
    or a
    jp nz,memoryNoMem
    ld a,(bankPageCount)
    cp 2
    jp c,memoryNoMem
    ld a,b
    ld (pressureTarget),a
    xor a
    ld (pressureRefusal),a
    ld hl,pressureTried
    ld bc,MODULE_MAX
    call cliZero
pressureChoose:
    ld a,255
    ld (pressureBest),a
    ld hl,MODULE_TABLE
    ld b,MODULE_MAX
pressureScan:
    push bc
    push hl
    ld a,(hl)
    inc hl
    or (hl)
    jp z,pressureNext
    inc hl
    inc hl
    ld c,(hl)
    ld a,c
    cp 2
    jp c,pressureNext
    ld a,(pressureTarget)
    cp MEM_ALLOC_ANY
    jp z,pressureMatch
    cp c
    jp nz,pressureNext
pressureMatch:
    dec hl
    dec hl
    dec hl
    call residentKey
    jp nz,pressureCandidate
    push hl
    call pressureSlot
    ld hl,residentManaged
    call pressureByte
    ld a,(hl)
    or a
    pop hl
    jp z,pressureNext
pressureCandidate:
    push hl
    call pressureSlot
    ld hl,pressureTried
    call pressureByte
    ld c,(hl)
    inc c
    dec c
    pop hl
    jp nz,pressureNext
    push hl
    ld hl,residentPriorities
    call pressureByte
    ld c,(hl)
    pop hl
    ld a,(pressureBest)
    cp c
    jp c,pressureNext
    jp z,pressureNext
    ld a,c
    ld (pressureBest),a
    ld (pressureRecord),hl
pressureNext:
    pop hl
    ld de,MODULE_RECORD_SIZE
    add hl,de
    pop bc
    dec b
    jp nz,pressureScan
    ld a,(pressureBest)
    cp 255
    jp z,pressureTryDisk
    ld hl,(pressureRecord)
    call pressureSlot
    ld (pressureIndex),a
    ld hl,pressureTried
    call pressureByte
    inc (hl)
    ld hl,residentVetoCodes
    call pressureByte
    ld a,(hl)
    or a
    jp z,pressureCanRelease
    push af
    ld a,(pressureIndex)
    add a,a
    ld hl,residentVetoFlags
    call pressureByte
    ld e,(hl)
    inc hl
    ld d,(hl)
    ld a,d
    or e
    jp z,pressureVetoActive
    ld hl,(pressureRecord)
    inc hl
    inc hl
    inc hl
    ld b,(hl)
    ex de,hl
    ld a,MEM_OP_GET8
    call mx_memory
    jp c,pressureVetoActive
    ld a,e
    or a
    jp nz,pressureVetoActive
    pop af
    jp pressureCanRelease
pressureVetoActive:
    pop af
    jp pressureRememberVeto
pressureCanRelease:
    ld hl,(pressureRecord)
    call mdCanRelease
    jp c,pressureChoose
    call klReclaim
    jp c,pressureChoose
    ld a,(pressureIndex)
    ld hl,residentReloadSlots
    call pressureByte
    ld a,(hl)
    or a
    jp z,pressureDrop
    dec a
    ld hl,residentReloadStatus
    call pressureByte
    ld (hl),2
pressureDrop:
    ld hl,(pressureRecord)
    jp mdReleaseChecked
pressureRememberVeto:
    ld (pressureRefusal),a
    ld hl,(pressureRecord)
    inc hl
    inc hl
    inc hl
    ld a,(hl)
    ld (pressureRefusalBank),a
    ld a,(pressureIndex)
    add a,a
    ld hl,residentVetoTexts
    call pressureByte
    ld e,(hl)
    inc hl
    ld d,(hl)
    ex de,hl
    ld (pressureRefusalName),hl
pressureFindReason:
    ld a,(kdPage)
    push af
    ld a,(pressureRefusalBank)
    ld (kdPage),a
    ld de,KD_DATA
    ld bc,13*256+1
    call kdReadString
    pop de
    ld a,d
    ld (kdPage),a
    jp nc,pressureReasonReady
pressureReasonInvalid:
    xor a
    ld (pressureRefusal),a
    jp pressureChoose
pressureReasonReady:
    ld (pressureRefusalText),hl
    jp pressureChoose
pressureTryDisk:
    call memoryFindFreeSlot
    jp c,pressureExhausted
    ld a,(memoryRamOwner)
    cp DOS_IO_PAGE
    jp nz,pressureExhausted
    ld a,(memoryRamPages)
    or a
    jp z,pressureExhausted
    inc a
    ld c,a
    ld a,(pressureTarget)
    cp MEM_ALLOC_ANY
    jp z,pressureSuspendDisk
    cp 2
    jp c,pressureExhausted
    cp c
    jp c,pressureSuspendDisk
    jp z,pressureSuspendDisk
pressureExhausted:
    ld a,(pressureRefusal)
    or a
    jp z,memoryNoMem
    ld c,a
    ld a,(pressureRefusalBank)
    ld b,a
    ld hl,(pressureRefusalText)
    ex de,hl
    ld hl,(pressureRefusalName)
    ld a,RESIDENT_APP_CANT_UNLOAD
    scf
    ret
pressureTarget: DB 0

;----------------------------------------------------------------------------
; MXOS
; После выхода приложения загрузить ожидающие резиденты на свободные места.
; вход: ресурсы вышедшей программы освобождены. Параметров в регистрах нет.
; выход: kdBusy, kdPage и выбор диска сохранены; AF/BC/DE/HL рабочие.
; Неудавшаяся загрузка остаётся в очереди. Вложенное вытеснение запрещено.
;----------------------------------------------------------------------------
pressureRestore:
    ld a,(pressureBusy)
    or a
    ret nz
    ld hl,residentReloadStatus
    ld b,pressureSlotCount
pressureCheckPending:
    ld a,(hl)
    cp 2
    jp z,pressureStartRestore
    inc hl
    dec b
    jp nz,pressureCheckPending
    ld a,(pressurePages)
    or a
    ret z
pressureStartRestore:
    ld a,1
    ld (pressureBusy),a
    ld a,pressureSlotCount
    ld (pressureReloadIndex),a
    ld a,(kdBusy)
    push af
    xor a
    ld (kdBusy),a
    ld a,(kdPage)
    push af
    ld a,(v_drive)
    push af
    call pressureRestoreDisk
    ld b,pressureSlotCount
    ld hl,residentReloadStatus
pressureRestoreNext:
    ld a,(hl)
    cp 2
    jp nz,pressureRestoreSkip
    push hl
    push bc
    ld a,pressureSlotCount
    sub b
    ld (pressureReloadIndex),a
    call pressurePath
    ld a,(hl)
    ld (pressureInitRegs+4),a
    ld a,MODULE_TARGET_BANK
    MXOS_CALL_REGS MXOS_DOS_MODULE_LOAD
    jp c,pressureRestorePop
    push hl
    ld de,pressureInitRegs
    ld a,RESIDENT_RELOAD
    ld (pressureInitRegs+7),a
    ld a,2
    ld (pressureInitRegs+5),a
    MXOS_CALL_REGS MXOS_DOS_MODULE_CALL
    pop hl
    jp c,pressureReloadFailed
    ld a,(pressureInitRegs+6)
    rrca
    jp c,pressureReloadFailed
    ld a,1
    MXOS_CALL_REGS MXOS_DOS_MODULE_UNLOAD
    jp pressureRestorePop
pressureReloadFailed:
    xor a
    MXOS_CALL_REGS MXOS_DOS_MODULE_UNLOAD
pressureRestorePop:
    pop bc
    pop hl
pressureRestoreSkip:
    inc hl
    dec b
    jp nz,pressureRestoreNext
    pop af
    call fileSelectDrive
    pop af
    ld (kdPage),a
    pop af
    ld (kdBusy),a
    xor a
    ld (pressureBusy),a
    ret
pressureReloadIndex: DB 0
pressureInitRegs:    DS MX_REGS,0

;----------------------------------------------------------------------------
; MXOS
; Проверить состав корня RAM-диска штатным итератором FAT.
; вход: зарегистрированный пул memoryRamOwner, kernelRamDrive.
; выход: CF=1, B:C и HL/DE описывают отказ. При недоступной геометрии или
; ошибке чтения память тома сохраняется. AF/BC/DE/HL рабочие.
;----------------------------------------------------------------------------
pressureSuspendDisk:
    ld a,(v_drive)
    push af
    call klReclaim
    jp c,pressureDiskUnsafe
    call flushVolume
    jp c,pressureDiskUnsafe
    ld a,(kernelRamDrive)
    call fileChangeDrive
    jp c,pressureDiskUnsafe
    call flushVolume
    jp c,pressureDiskUnsafe
    call mountVolume
    jp c,pressureDiskUnsafe
    ld hl,(v_directory)
    push hl
    ld hl,0
    ld (v_directory),hl
    call fatDirStart
    jp c,pressureDiskScanFailed
    ld a,8
    call fatDirFiltered
    pop hl
    ld (v_directory),hl
    jp nc,pressureDiskNotEmpty
    or a
    jp nz,pressureDiskUnsafe
    jp pressureDiskEmpty
pressureDiskScanFailed:
    pop hl
    ld (v_directory),hl
pressureDiskUnsafe:
    ld hl,pressureDiskUnknown
    ld c,2
    jp pressureDiskRefuses
pressureDiskNotEmpty:
    ld hl,pressureDiskFiles
    ld c,1
pressureDiskRefuses:
    ld (pressureRefusalText),hl
    ld a,c
    ld (pressureRefusal),a
    ld a,DOS_IO_PAGE
    ld (pressureRefusalBank),a
    ld hl,pressureDiskName
    ld (pressureRefusalName),hl
    pop af
    call fileSelectDrive
    jp pressureExhausted
pressureDiskName:    DB 'RAMDISK.COM',0
pressureDiskFiles:   DB 'RAM disk is not empty',0
pressureDiskUnknown: DB 'Cannot verify RAM disk',0
pressureDiskEmpty:
    ld a,(memoryRamPages)
    ld (pressurePages),a
    xor a
    call pressurePublishPages
    pop af
    call fileSelectDrive
    xor a
    ret

;----------------------------------------------------------------------------
; MXOS
; Отразить владение пулом в распределителе и реестре физического драйвера.
; вход: A=число доступных RAM-диску страниц. Запись на том завершена.
; выход: геометрия реестра и memoryRamPages согласованы. Кэш этой буквы и
; подготовленные прямые серии недействительны. AF/BC/DE/HL рабочие.
;----------------------------------------------------------------------------
pressurePublishPages:
    ld (memoryRamPages),a
    ld b,a
    ld a,(kernelRamDrive)
    call registryRecord
    inc hl
    ld (hl),b
    call klInvalidate
    ld a,(kernelRamDrive)
    call invalidateDriveCache
    ld hl,v_drive
    cp (hl)
    ret nz
    jp selectVolume

;----------------------------------------------------------------------------
; MXOS
; Заново создать ранее проверенный пустой RAM-диск штатным FORMAT.
; вход: pressurePages ненулевой только после успешной проверки корня. Все
; страницы исходного пула должны быть свободны, MAIN и установщик неактивны.
; выход: при отказе сохраняется заявка, неготовый том снова закрыт. AF/BC/DE/HL
; рабочие. Внешний диспетчер восстановления сохраняет диск и состояние DOS.
;----------------------------------------------------------------------------
pressureRestoreDisk:
    ld a,(pressurePages)
    or a
    ret z
    inc a
    ld c,a
    ld a,(appDepth)
    or a
    ret nz
    ld a,(moduleActor)
    or a
    ret nz
    ld a,(diActive)
    or a
    ret nz
    ld hl,MEM_ALLOC_TABLE
    ld b,MEM_ALLOC_RECORDS
    ld de,6
pressureCheckPage:
    ld a,(hl)
    cp 2
    jp c,pressureNextPage
    cp c
    ret c
    ret z
pressureNextPage:
    add hl,de
    dec b
    jp nz,pressureCheckPage
    ld a,(kernelRamDrive)
    add a,'A'
    ld (pressureFormatDrive),a
    ld a,(pressurePages)
    call pressurePublishPages
    xor a
    ld (kdBusy),a
    ld hl,pressureFormatCommand
    call keExternal
    jp c,pressureFormatFailed
    ld a,(kernelRamDrive)
    call fileSelectDrive
    call mountVolume
    jp c,pressureFormatFailed
    call fatCheckMounted
    jp c,pressureFormatFailed
    ld hl,(v_diskInfo.totalSectors)
    ld a,(pressurePages)
    cp h
    jp nz,pressureFormatFailed
    ld a,l
    or a
    jp nz,pressureFormatFailed
    ld (pressurePages),a
    ret
pressureFormatFailed:
    xor a
    jp pressurePublishPages
pressureFormatCommand: DB BOOT_VOLUME_LETTER,':',05Ch,'DOS',05Ch,'FORMAT.COM '
pressureFormatDrive:   DB 'B:',32,'-R -Y',0
