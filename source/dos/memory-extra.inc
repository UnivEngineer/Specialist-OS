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
memoryFillSize: DW 0
memoryFillDone: DW 0
memoryFillCopy: DS 8,0

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
    ld hl,0FF00h
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
    ld (memoryQueryEnd),hl
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
memoryQueryBank: DB 0
memoryQueryCursor: DW 0
memoryQueryEnd: DW 0
memoryQueryNext: DW 0
memoryQueryBlockEnd: DW 0
memoryQueryTotal: DW 0
memoryQueryMax: DW 0

; C=0: B=размер пула, C=0 свободен/1 DOS/2 старый резидент.
; C=1: DRIVER-инициализатор передаёт подготовленный пул самой DOS.
; Никакой модульный handle и реестр FAR-блоков для этого не нужны.
memoryRamDisk:
    ld a,c
    or a
    jp z,memoryRamDiskStatus
    cp MEM_RAMDISK_COMMIT
    jp nz,memoryBad
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
    ld a,DOS_IO_PAGE
    ld (memoryRamOwner),a
memoryRamDiskStatus:
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
