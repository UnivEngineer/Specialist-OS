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
    jp memoryResidentReset

; One machine-wide lease. Store owner+1 so the anonymous MAIN owner is valid.
timerReleaseOwner:       ; C = allocation owner, preserved
    ld a,(timerOwner)
    dec a
    cp c
    ret nz
    xor a
    ld (timerOwner),a
    ret
timerService:
    ld b,a
    call memoryOwnerCurrent
    inc a
    ld c,a
    ld hl,timerOwner
    ld a,b
    cp TIMER_QUERY
    jp z,timerQuery
    cp TIMER_RELEASE
    jp z,timerRelease
    jp nc,memoryBad
    ld a,(hl)
    or a
    jp nz,memoryDenied
    ld (hl),c
    ld a,b
    or a
    ret z               ; exclusive raw access, no PIT writes
    call pitTimerInit
    ret nc
    xor a
    ld (timerOwner),a
    jp memoryUnsupported
timerRelease:
    ld a,(hl)
    cp c
    jp nz,memoryDenied
    xor a
    ld (hl),a
    ret
timerQuery:
    ld a,(hl)
    or a
    ret z
    cp c
    ld a,1
    ret z
    inc a
    or a
    ret

    DEFINE TIMING_PIT_INPUT_CLOCK
    INCLUDE "../include/timing-pit.inc"
    UNDEFINE TIMING_PIT_INPUT_CLOCK

dosMemoryService:
    cp MEM_OP_MAINRETIRE
    jp z,memoryRetireMain
    cp MEM_OP_MAINREGISTER
    jp z,memoryRegisterMain
    cp MEM_OP_MAINCLAIM
    jp z,memoryMainClaim
    cp MEM_OP_RESIDENT
    jp z,memoryResident
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
    ; MAIN allocations of a resident remain ordinary tracked resources. Only
    ; system reservations (FONT) promise survival against MAIN replacement.
    ld a,b
    or a
    jp nz,memoryBad
    ld a,h
    or a
    jp z,memoryBad
    push hl
    add hl,de
    jp c,memoryArenaBad
    push hl
    ld hl,(memorySystemEnd)
    ex de,hl
    pop hl
    call cmp_hl_de
    jp c,memoryArenaRange
    jp nz,memoryArenaBad
memoryArenaRange:
    ld (memoryNewEnd),hl
    ; Нельзя менять арену, пока из неё выданы блоки.
    ld hl,MEM_ALLOC_TABLE
    ld c,MEM_ALLOC_RECORDS
memoryArenaScan:
    push hl
    push de
    ex de,hl
    ld hl,(appImage)
    call cmp_hl_de
    ex de,hl
    pop de
    pop hl
    jp z,memoryArenaScanNext
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

; Register an existing static buffer through the normal allocation ledger.
; Exact ranges may be odd-aligned. Never replace another allocation, even own.
memoryRegisterMain:
    ld a,b
    or a
    jp nz,memoryBad
    ld a,h
    or l
    jp z,memoryBad
    ld a,d
    or e
    jp z,memoryBad
    ld (memoryCandidate),hl
    ex de,hl
    ld (memorySize),hl
    ex de,hl
    add hl,de
    jp c,memoryBad
    ld (memoryNewEnd),hl
    ex de,hl
    ld hl,(memorySystemEnd)
    call cmp_hl_de
    jp c,memoryBad
    xor a
    ld (memoryBank),a
    ld a,0FEh
    ld (memoryPlacement),a
    jp memoryFindSlot
memoryAlloc:
    ld (memorySize),hl
    ld a,h
    or l
    jp z,memoryNull
    ld a,b
    ld (memoryBank),a
    ld (memoryPlacement),a
memoryFindSlot:
    ; Сначала резервируем запись; таблица не меняется до успешного поиска.
    call memoryFindFreeSlot
    ret c
    jp memorySlotFound
memoryFindFreeSlot:
    ld hl,MEM_ALLOC_TABLE
    ld c,MEM_ALLOC_RECORDS
memorySlot:
    ld a,(hl)
    cp 0FFh
    ret z
    ld de,6
    add hl,de
    dec c
    jp nz,memorySlot
    jp memoryNoMem
memorySlotFound:
    ld (memorySlotPtr),hl
    ld a,(memoryPlacement)
    cp 0FEh ; Private exact-placement mode of MAINREGISTER, never a FAR bank.
    jp z,memoryScanStart
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
    ld hl,MEM_DATA_LIMIT
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
    call memoryCapMain
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
    ld a,(memoryPlacement)
    cp 0FEh
    jp z,mdActive
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
    call memoryWriteRecord
    call memoryOwnerCurrent
    ld (hl),a
    ld a,(memoryPlacement)
    cp 0FEh
    call z,memoryPublishTop
    ld a,(memoryBank)
    ld b,a
    ld c,0
    ld hl,(memorySize)
    ex de,hl
    ld hl,(memoryCandidate)
    xor a
    ret
; Fill the bank, base and size. Return HL pointing to the owner byte.
memoryWriteRecord:
    ld a,(memoryBank)
    or a
    call z,mdReuseInvalidateAllocation
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

; Internal fixed reservation [HL,DE), already checked by the MAIN arena.
; Unlike public ALLOC this permits address 0000h. Allocation record stays
; private (owner FFh) until the loader attaches it to an application/module.
; HL=base, B=0, memorySlotPtr=record; CF/A=failure, no partial publication.
memoryReserveMain:
    ld (memoryCandidate),hl
    ex de,hl
    call sub_hl_de
    ld (memorySize),hl
    call memoryFindFreeSlot
    ret c
    ld (memorySlotPtr),hl
    xor a
    ld (memoryBank),a
    call memoryWriteRecord
    ld (hl),0FFh
    ld hl,(memoryCandidate)
    ld b,0
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
    call memoryPublishTop
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
    ; Bootstrap проверил хвост MEM до его восстановления: это новое ОЗУ.
    ld a,(memoryRamFresh)
    or a
    jp nz,memoryApplyLimit
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
    ld b,a
    ld a,(kernelRamDrive)
    call registryRecord
    inc hl
    ld (hl),b
    xor a
    ret
; После перезагрузки DOS готовый B: мог уже иметь запрошенный размер.
; Геометрию проверяет штатное монтирование FAT; иной том не переразмечаем.
memoryExistingVolume:
    ld a,(v_drive)
    push af
    ld a,(kernelRamDrive)
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

    INCLUDE "memory-extra.inc"
    INCLUDE "memory-resident.inc"
