; Менеджер FAR-блоков: 32 записи по 6 байт, без заголовков в данных банка.
; Владелец — текущий модуль либо банк инициатора; {bank,offset,size,owner}.
; Память main выдаётся только из явно зарегистрированной арены.
; ALLOC B=банк/FFh любой свободный DATA, HL=байты -> B:HL, DE=размер.
; FREE B:HL; REALLOC B:HL, DE=новый размер -> B:HL. При отказе старый блок жив.
; ARENA B=0, HL=начало, DE=размер: доверенная регистрация свободной main-арены.
; RAMLIMIT B=число банков B:, менять можно лишь на пустом RAM-диске.
memoryInit:
    ld hl,memcpy_bc_hl
    ld (MEM_NEAR_COPY),hl
    ld hl,MEM_ALLOC_TABLE
    ld b,MEM_ALLOC_RECORDS
memoryInitLoop:
    ld (hl),0FFh
    ld de,6
    add hl,de
    dec b
    jp nz,memoryInitLoop
    ld a,(bankPageCount)
    dec a
    ld (memoryRamPages),a
    xor a
    ld (memoryRamOwner),a
    ld hl,0
    ld (memoryMainStart),hl
    ld (memoryMainEnd),hl
    ret

dosMemoryService:
    cp MEM_OP_RAMDISK
    jp z,memoryRamDisk
    cp MEM_OP_DISKCLAIM
    jp z,memoryDiskClaim
    cp MEM_OP_FILL
    jp z,memoryFill
    cp MEM_OP_QUERY
    jp z,memoryQuery
    cp MEM_OP_ALLOC
    jp z,memoryAlloc
    cp MEM_OP_FREE
    jp z,memoryFree
    cp MEM_OP_REALLOC
    jp z,memoryRealloc
    cp MEM_OP_ARENA
    jp z,memoryArena
    cp MEM_OP_RAMLIMIT
    jp z,memoryRamLimit
    cp MEM_OP_MOVE
    jp z,memoryUnsupported
    cp MEM_OP_PRINT
    jp nc,memoryUnsupported
    jp mx_memory
memoryUnsupported:
    ld a,DOS_ERR_UNSUPPORTED
    scf
    ret
memoryNoMem:
    ld a,MEM_ERROR_NOMEM
    scf
    ret
memoryDenied:
    ld a,MEM_ERROR_OWNER
    scf
    ret
memoryBad:
    ld a,MEM_ERROR_RANGE
    scf
    ret

memoryArena:
    call residentActorCheck
    jp nz,memoryDenied
    ld a,b
    or a
    jp nz,memoryBad
    ld a,h
    or a
    jp z,memoryBad
    push hl
    add hl,de
    jp c,memoryArenaBad
    ld a,h
    cp 08Ch
    jp c,memoryArenaRange
    jp nz,memoryArenaBad
    ld a,l
    or a
    jp nz,memoryArenaBad
memoryArenaRange:
    ld (memoryNewEnd),hl
    ; Нельзя менять арену, пока из неё выданы блоки.
    ld hl,MEM_ALLOC_TABLE
    ld c,MEM_ALLOC_RECORDS
memoryArenaScan:
    ld a,(hl)
    or a
    jp nz,memoryArenaScanNext
    push hl
    ld de,5
    add hl,de
    call memoryOwnerCurrent
    cp (hl)
    pop hl
    jp z,memoryArenaBusy
memoryArenaScanNext:
    ld de,6
    add hl,de
    dec c
    jp nz,memoryArenaScan
    pop hl
    ld (memoryMainStart),hl
    ld hl,(memoryNewEnd)
    ld (memoryMainEnd),hl
    call memoryOwnerCurrent
    ld (memoryMainOwner),a
    xor a
    ret
memoryArenaBad:
    pop hl
    jp memoryBad
memoryArenaBusy:
    pop hl
    jp memoryDenied

memoryAlloc:
    ld (memorySize),hl
    ld a,h
    or l
    jp z,memoryNull
    ld a,b
    ld (memoryBank),a
    ld (memoryPlacement),a
    ; Сначала резервируем запись; таблица не меняется до успешного поиска.
    ld hl,MEM_ALLOC_TABLE
    ld c,MEM_ALLOC_RECORDS
memorySlot:
    ld a,(hl)
    inc a
    jp z,memorySlotFound
    ld de,6
    add hl,de
    dec c
    jp nz,memorySlot
    jp memoryNoMem
memorySlotFound:
    ld (memorySlotPtr),hl
    ld a,(memoryBank)
    or a
    jp z,memoryMain
    cp MEM_ALLOC_ANY
    jp nz,memoryBankCheck
    ld a,(memoryRamPages)
    add a,2
memoryBankCheck:
    ld (memoryBank),a
    ld b,a
    ld a,(bankPageCount)
    cp b
    jp c,memoryNoMem
    ld a,(memoryRamPages)
    inc a
    cp b
    jp nc,memoryDenied
    ld hl,0
    ld (memoryCandidate),hl
    ld hl,0FF00h
    ld (memoryLimit),hl
    jp memoryTry
memoryMain:
    ld a,(memoryMainOwner)
    ld b,a
    call memoryOwnerCurrent
    cp b
    jp nz,memoryDenied
    ld hl,(memoryMainStart)
    ld a,h
    or l
    jp z,memoryNoMem
    ld (memoryCandidate),hl
    ld hl,(memoryMainEnd)
    ld (memoryLimit),hl
memoryTry:
    ; Выравнивание два байта, одинаковое для C/ASM.
    ld hl,(memoryCandidate)
    ld a,l
    and 1
    jp z,memoryAligned
    inc hl
    ld (memoryCandidate),hl
memoryAligned:
    push hl
    ld hl,(memorySize)
    ex de,hl
    pop hl
    add hl,de
    jp c,memoryNextBank
    ld (memoryNewEnd),hl
    push hl
    ld hl,(memoryLimit)
    ex de,hl
    pop hl
    call cmp_hl_de
    jp c,memoryScanStart
    jp z,memoryScanStart
    jp memoryNextBank
memoryScanStart:
    ld hl,MEM_ALLOC_TABLE
    ld c,MEM_ALLOC_RECORDS
memoryScan:
    ld a,(memoryBank)
    cp (hl)
    jp nz,memoryScanNext
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
    ld (memoryBlockEnd),hl
    ld hl,(memoryCandidate)
    ex de,hl
    ld hl,(memoryBlockEnd)
    call cmp_hl_de
    pop de
    jp c,memoryNoOverlap
    jp z,memoryNoOverlap
    ld hl,(memoryNewEnd)
    call cmp_hl_de
    jp c,memoryNoOverlap
    jp z,memoryNoOverlap
    pop hl
    ld hl,(memoryBlockEnd)
    ld (memoryCandidate),hl
    jp memoryTry
memoryNoOverlap:
    pop hl
memoryScanNext:
    ld de,6
    add hl,de
    dec c
    jp nz,memoryScan
    ld hl,(memorySlotPtr)
    ld a,(memoryBank)
    ld (hl),a
    inc hl
    push hl
    ld hl,(memoryCandidate)
    ex de,hl
    pop hl
    ld (hl),e
    inc hl
    ld (hl),d
    inc hl
    push hl
    ld hl,(memorySize)
    ex de,hl
    pop hl
    ld (hl),e
    inc hl
    ld (hl),d
    inc hl
    call memoryOwnerCurrent
    ld (hl),a
    ld a,(memoryBank)
    ld b,a
    ld c,0
    ld hl,(memoryCandidate)
    xor a
    ret
memoryNextBank:
    ld a,(memoryPlacement)
    cp MEM_ALLOC_ANY
    jp nz,memoryNoMem
    ld a,(memoryBank)
    or a
    jp z,memoryNoMem
    inc a
    jp memoryBankCheck
memoryNull:
    ld hl,0
    ld bc,0
    ld de,0
    xor a
    ret

; B:HL -> запись; возвращает HL=начало, DE=размер. Владелец проверяется.
memoryFind:
    ld (memoryOldAddress),hl
    ld hl,MEM_ALLOC_TABLE
    ld c,MEM_ALLOC_RECORDS
memoryFindLoop:
    ld a,(hl)
    cp b
    jp nz,memoryFindNext
    push hl
    inc hl
    ld e,(hl)
    inc hl
    ld d,(hl)
    ld hl,(memoryOldAddress)
    call cmp_hl_de
    pop hl
    jp nz,memoryFindNext
    push hl
    ld de,5
    add hl,de
    call memoryOwnerCurrent
    cp (hl)
    pop hl
    jp nz,memoryDenied
    push hl
    inc hl
    inc hl
    inc hl
    ld e,(hl)
    inc hl
    ld d,(hl)
    pop hl
    xor a
    ret
memoryFindNext:
    ld de,6
    add hl,de
    dec c
    jp nz,memoryFindLoop
    jp memoryDenied
memoryFree:
    ld a,b
    or h
    or l
    ret z
    call memoryFind
    ret c
    ld (hl),0FFh
    xor a
    ret

memoryRealloc:
    ex de,hl
    ld (memoryResize),hl
    ex de,hl
    ld (memoryOldAddress),hl
    ld a,b
    ld (memoryOldBank),a
    or h
    or l
    jp nz,memoryResizeExisting
    ex de,hl
    ld b,MEM_ALLOC_ANY
    jp memoryAlloc
memoryResizeExisting:
    ld a,d
    or e
    jp z,memoryResizeZero
    call memoryFind
    ret c
    ld (memoryOldSlot),hl
    ex de,hl
    ld (memoryOldSize),hl
    ex de,hl
    ld hl,(memoryResize)
    ld a,d
    cp h
    jp c,memoryResizeGrow
    jp nz,memoryResizeShrink
    ld a,e
    cp l
    jp c,memoryResizeGrow
memoryResizeShrink:
    ; Сжатие на месте не копирует данные и сохраняет адрес.
    ex de,hl
    ld hl,(memoryOldSlot)
    inc hl
    inc hl
    inc hl
    ld (hl),e
    inc hl
    ld (hl),d
    ld hl,(memoryOldAddress)
    xor a
    ret
memoryResizeGrow:
    ld a,(memoryOldBank)
    or a
    jp z,memoryResizeAllocate
    ld b,MEM_ALLOC_ANY
memoryResizeAllocate:
    call memoryAlloc
    ret c
    ld (memoryCopy+4),hl
    ld a,b
    ld (memoryCopy+3),a
    ld hl,(memoryOldAddress)
    ld (memoryCopy+1),hl
    ld hl,(memoryOldSize)
    ld (memoryCopy+6),hl
    ld a,(memoryOldBank)
    ld (memoryCopy),a
    ld hl,memoryCopy
    call mx_memcpyFar
    jp c,memoryResizeRollback
    ld hl,(memoryOldSlot)
    ld (hl),0FFh
    ld hl,(memoryCopy+4)
    push hl
    ld hl,(memoryResize)
    ex de,hl
    pop hl
    xor a
    ret
memoryResizeRollback:
    push af
    ld hl,(memorySlotPtr)
    ld (hl),0FFh
    pop af
    ret
memoryResizeZero:
    call memoryFree
    ret c
    jp memoryNull

memoryRamLimit:
    ld a,(memoryRamPages)
    cp b
    ret z
    ld a,(memoryRamOwner)
    or a
    jp nz,memoryDenied
    ld a,(bankPageCount)
    dec a
    cp b
    jp c,memoryBad
    push hl
    ld h,b
    ld l,c
    ld (memoryRequestedPages),hl
    pop hl
    ; Изменение геометрии допустимо только до разметки и без FAR-блоков.
    ld hl,MEM_ALLOC_TABLE
    ld c,MEM_ALLOC_RECORDS
memoryLimitScan:
    ld a,(hl)
    inc a
    jp nz,memoryDenied
    ld de,6
    add hl,de
    dec c
    jp nz,memoryLimitScan
    ld hl,KD_DATA
    ld de,0
    ld b,2
    call dosRamDriver
    ret c
    ld hl,KD_DATA
    ld b,0
memoryBlankScan:
    ld a,(hl)
    or a
    jp z,memoryBlankNext
    inc a
    jp nz,memoryExistingVolume
memoryBlankNext:
    inc hl
    dec b
    jp nz,memoryBlankScan
memoryApplyLimit:
    ld a,(memoryRequestedPages+1)
    ld (memoryRamPages),a
    ld (DOS_DRIVER_REGISTRY+DRIVER_RECORD_SIZE+1),a
    xor a
    ret
; После перезагрузки DOS готовый B: мог уже иметь запрошенный размер.
; Геометрию проверяет штатное монтирование FAT; иной том не переразмечаем.
memoryExistingVolume:
    ld a,(v_drive)
    push af
    ld a,1
    call fileChangeDrive
    jp c,memoryVolumeResult
    ld hl,(v_diskInfo.sectorSize)
    ld de,256
    call cmp_hl_de
    jp nz,memoryVolumeDenied
    ld hl,(v_diskInfo.totalSectors)
    ld a,(memoryRequestedPages+1)
    ld d,a
    ld e,0
    call cmp_hl_de
    jp z,memoryVolumeResult
memoryVolumeDenied:
    scf
memoryVolumeResult:
    push af
    pop bc
    pop af
    push bc
    call fileSelectDrive
    pop af
    jp c,memoryDenied
    jp memoryApplyLimit
memoryRamOwner: DB 0
memoryRamPages: DB 0
memoryRequestedPages: DW 0
memoryMainOwner: DB 0
memoryMainStart: DW 0
memoryMainEnd: DW 0
memoryPlacement: DB 0
memoryBank: DB 0
memorySize: DW 0
memoryCandidate: DW 0
memoryLimit: DW 0
memoryNewEnd: DW 0
memoryBlockEnd: DW 0
memorySlotPtr: DW 0
memoryOldSlot: DW 0
memoryOldBank: DB 0
memoryOldAddress: DW 0
memoryOldSize: DW 0
memoryResize: DW 0
memoryCopy: DS 8,0

    INCLUDE "memory-extra.inc"
