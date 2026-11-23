; FILL: B=банк, HL=начало, DE=длина, C=байт. Нулевая длина ничего не меняет.
; Проверяем весь диапазон до первой записи. Регистры разрушены, CF/A=статус.
memoryFill:
    ld a,d
    or e
    ret z
    ld a,b
    push bc
    call bankCopyPageCheck
    pop bc
    ret c
    push hl
    add hl,de
    jp c,memoryFillBad
    ld a,h
    cp 0FFh
    jp c,memoryFillRange
    jp nz,memoryFillBad
    ld a,l
    or a
    jp nz,memoryFillBad
memoryFillRange:
    pop hl
    push hl
    ex de,hl
    ld (memoryFillSize),hl
    ex de,hl
    pop hl
    ld (memoryFillCopy+1),hl
    ld a,b
    ld (memoryFillCopy),a
    ld (memoryFillCopy+3),a
    ld e,c
    ld a,MEM_OP_PUT8
    call mx_memory
    ret c
    ld hl,1
    ld (memoryFillDone),hl
memoryFillLoop:
    ld hl,(memoryFillDone)
    ex de,hl
    ld hl,(memoryFillSize)
    call sub_hl_de
    ld a,h
    or l
    ret z
    call cmp_hl_de
    jp c,memoryFillCount
    ex de,hl
memoryFillCount:
    ld (memoryFillCopy+6),hl
    ld hl,(memoryFillCopy+1)
    push hl
    ld hl,(memoryFillDone)
    ex de,hl
    pop hl
    add hl,de
    ld (memoryFillCopy+4),hl
    ld hl,memoryFillCopy
    call mx_memcpyFar
    ret c
    ld hl,(memoryFillCopy+6)
    push hl
    ld hl,(memoryFillDone)
    ex de,hl
    pop hl
    add hl,de
    ld (memoryFillDone),hl
    jp memoryFillLoop
memoryFillBad:
    pop hl
    jp memoryBad

; QUERY: B=банк; HL=свободные байты, DE=наибольший промежуток,
; B=банк, C=свободные записи реестра. AF разрушен, CF/A=статус.
; Сведения о main относятся к текущей доверенной арене.
memoryQuery:
    ld a,b
    call bankCopyPageCheck
    ret c
    ld a,b
    ld (memoryQueryBank),a
    ld hl,0
    ld (memoryQueryTotal),hl
    ld (memoryQueryMax),hl
    ld (memoryQueryCursor),hl
    ld (memoryQueryEnd),hl
    ld a,b
    or a
    jp z,memoryQueryMain
    ld a,(memoryRamPages)
    inc a
    cp b
    jp nc,memoryQuerySlots
    ld hl,MEM_DATA_LIMIT
    ld (memoryQueryEnd),hl
    jp memoryQueryGap
memoryQueryMain:
    ld a,(memoryMainOwner)
    ld c,a
    call memoryOwnerCurrent
    cp c
    jp nz,memoryDenied
    ld hl,(memoryMainStart)
    ld (memoryQueryCursor),hl
    ld hl,(memoryMainEnd)
    call memoryCapMain
    ld (memoryQueryEnd),hl
    ex de,hl
    ld hl,(memoryQueryCursor)
    call cmp_hl_de
    jp c,memoryQueryGap
    ex de,hl
    ld (memoryQueryCursor),hl
memoryQueryGap:
    ld hl,(memoryQueryEnd)
    ld (memoryQueryNext),hl
    ld hl,MEM_ALLOC_TABLE
    ld b,MEM_ALLOC_RECORDS
memoryQueryScan:
    ld a,(memoryQueryBank)
    cp (hl)
    jp nz,memoryQuerySkip
    push hl
    inc hl
    ld e,(hl)
    inc hl
    ld d,(hl)
    inc hl
    push de
    ld a,(hl)
    inc hl
    ld h,(hl)
    ld l,a
    add hl,de
    ld (memoryQueryBlockEnd),hl
    ld hl,(memoryQueryCursor)
    call cmp_hl_de
    jp c,memoryQueryAbove
    push hl
    ld hl,(memoryQueryBlockEnd)
    ex de,hl
    pop hl
    call cmp_hl_de
    jp nc,memoryQueryPast
    ; Курсор попал в занятый блок: начинаем поиск за ним.
    ex de,hl
    ld (memoryQueryCursor),hl
    pop de
    pop hl
    jp memoryQueryGap
memoryQueryAbove:
    ld hl,(memoryQueryNext)
    call cmp_hl_de
    jp c,memoryQueryPast
    ex de,hl
    ld (memoryQueryNext),hl
memoryQueryPast:
    pop de
    pop hl
memoryQuerySkip:
    ld de,6
    add hl,de
    dec b
    jp nz,memoryQueryScan
    ld hl,(memoryQueryCursor)
    ld a,l
    and 1
    jp z,memoryQueryAligned
    inc hl
memoryQueryAligned:
    ex de,hl
    ld hl,(memoryQueryNext)
    call cmp_hl_de
    jp c,memoryQueryNoGap
    call sub_hl_de
    push hl
    push hl
    ld hl,(memoryQueryTotal)
    ex de,hl
    pop hl
    add hl,de
    ld (memoryQueryTotal),hl
    pop hl
    push hl
    ld hl,(memoryQueryMax)
    ex de,hl
    pop hl
    call cmp_hl_de
    jp c,memoryQueryNoGap
    ld (memoryQueryMax),hl
memoryQueryNoGap:
    ld hl,(memoryQueryNext)
    push hl
    ld hl,(memoryQueryEnd)
    ex de,hl
    pop hl
    call cmp_hl_de
    jp nc,memoryQuerySlots
    ld (memoryQueryCursor),hl
    jp memoryQueryGap
memoryQuerySlots:
    ld hl,MEM_ALLOC_TABLE
    ld b,MEM_ALLOC_RECORDS
    ld c,0
memoryQuerySlot:
    ld a,(hl)
    inc a
    jp nz,memoryQueryUsed
    inc c
memoryQueryUsed:
    ld de,6
    add hl,de
    dec b
    jp nz,memoryQuerySlot
    ld a,(memoryQueryBank)
    ld b,a
    ld hl,(memoryQueryTotal)
    push hl
    ld hl,(memoryQueryMax)
    ex de,hl
    pop hl
    xor a
    ret

; C=0: B=размер пула, C=0 свободен/1 DOS/2 старый резидент; D=новое ОЗУ, E=0.
; C=1: DRIVER-инициализатор передаёт подготовленный пул самой DOS.
; Никакой модульный handle и реестр FAR-блоков для этого не нужны.
memoryRamDisk:
    ld a,c
    or a
    jp z,memoryRamDiskStatus
    cp MEM_RAMDISK_CHECK_LETTER
    jp z,memoryRamCheckLetter
    cp MEM_RAMDISK_COMMIT
    jp nz,memoryBad
    call memoryRamCheckLetter
    ret c
    ld a,b
    ld (ramRequestedDrive),a
    ld a,(diActive)
    or a
    jp z,memoryDenied
    ld a,(memoryRamOwner)
    or a
    jp z,memoryRamDiskValidate
    cp DOS_IO_PAGE
    jp nz,memoryDenied
memoryRamDiskValidate:
    ld a,(memoryRamPages)
    or a
    jp z,memoryBad
    ld (memoryRequestedPages+1),a
    ; Повторно проверяем геометрию B:, не форматируя и не меняя данные.
    call memoryExistingVolume
    ret c
    call memoryRamPublishLetter
    ret c
    ld a,DOS_IO_PAGE
    ld (memoryRamOwner),a
    xor a
    ld (memoryRamFresh),a
memoryRamDiskStatus:
    ld a,(memoryRamFresh)
    ld d,a
    ld a,(kernelRamDrive)
    ld e,a
    ld a,(memoryRamPages)
    ld b,a
    ld c,0
    ld a,(memoryRamOwner)
    or a
    ret z
    inc c
    cp DOS_IO_PAGE
    jp z,memoryRamDiskReady
    inc c
memoryRamDiskReady:
    xor a
    ret

; B = requested RAM letter. Check the installer, one-time placement and the
; target slot. B stays intact. CF/A report errors; AF and HL are scratch.
memoryRamCheckLetter:
    ld a,(diActive)
    or a
    jp z,memoryDenied
    ld a,(kernelRamDrive)
    cp b
    jp z,dosClearError
    cp RAM_DRIVE
    jp nz,memoryBad
    ld a,b
    cp RAM_ALTERNATE_DRIVE
    jp nz,memoryBad
    ld a,(memoryRamOwner)
    or a
    jp nz,memoryDenied
    ld a,(kernelFloppyDrive)
    cp b
    jp z,memoryDenied
    ld a,(kernelExternalDrive)
    cp b
    jp z,memoryDenied
    jp dosClearError

; After a successful mount, move the RAM registry from B: to D: if requested.
; CF/A report a flush failure before publication; all other registers are scratch.
memoryRamPublishLetter:
    ld a,(ramRequestedDrive)
    ld hl,kernelRamDrive
    cp (hl)
    jp z,dosClearError
    call flushVolume
    ret c
    ld hl,DOS_DRIVER_REGISTRY+RAM_DRIVE*DRIVER_RECORD_SIZE
    ld de,DOS_DRIVER_REGISTRY+RAM_ALTERNATE_DRIVE*DRIVER_RECORD_SIZE
    ld bc,DRIVER_RECORD_SIZE
    call moveBytesForward
    ld hl,DOS_DRIVER_REGISTRY+RAM_DRIVE*DRIVER_RECORD_SIZE
    ld bc,DRIVER_RECORD_SIZE
    call cliZero
    ld hl,diskDriverDummy
    ld (v_drives+2*RAM_DRIVE),hl
    ld hl,diskDriver
    ld (v_drives+2*RAM_ALTERNATE_DRIVE),hl
    ld a,RAM_ALTERNATE_DRIVE
    ld (kernelRamDrive),a
    call invalidateDriveCache
    ld a,RAM_DRIVE
    call invalidateDriveCache
    ld a,(v_drive)
    cp RAM_DRIVE
    jp nz,dosClearError
    ld a,RAM_ALTERNATE_DRIVE
    jp fileSelectDrive

; Исторический API: закрепить пул B: за модульным резидентом.
memoryDiskClaim:
    ld a,(moduleActor)
    or a
    jp z,memoryDenied
    ld b,a
    ld a,(memoryRamOwner)
    or a
    jp z,memoryDiskClaimNew
    cp b
    jp nz,memoryDenied
memoryDiskClaimNew:
    ld a,b
    sub MODULE_OWNER_BASE
    add a,a
    ld e,a
    ld d,0
    ld hl,residentKeys
    add hl,de
    ld a,(hl)
    inc hl
    or (hl)
    jp z,memoryDenied
    ld a,b
    ld (memoryRamOwner),a
    xor a
    ret
