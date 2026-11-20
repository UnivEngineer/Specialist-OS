; System-owned MAIN reservations. Unlike ALLOC these survive appLeave and
; moduleWarm. Slots identify services, not temporary application owners.
; B=0 query, B=1 replace; C=slot; HL=size (multiple of 256), DE=protected end.
; Size 0 releases the slot. FFFFh sizes and arbitrary alignment are rejected.
; Success: HL=base (0 if empty), DE=size, BC=inclusive memTop. CF/A=error.
; Trusted installer must supply the end of its live code/data/parent workspace.
; All existing MAIN allocations are checked independently. Nothing is moved.
memoryResident:
    ld a,c
    cp MEM_RESIDENT_SLOTS
    jp nc,memoryBad
    ld a,b
    cp 2
    jp nc,memoryBad
    ld (mrOperation),a
    ld (mrSize),hl
    ex de,hl
    ld (mrFloor),hl
    ld a,c
    add a,a
    add a,a
    ld l,a
    ld h,0
    ld de,mrSlots
    add hl,de
    ld (mrSlot),hl
    ld a,(mrOperation)
    or a
    jp z,mrResult
    ld hl,(mrSize)
    ld a,l
    or a
    jp nz,memoryBad
    ; Exclude the replaced slot when finding the next available top.
    ld hl,APP_MEMORY_END
    ld (mrCeiling),hl
    ld hl,mrSlots
    ld b,MEM_RESIDENT_SLOTS
mrCeilingLoop:
    push hl
    push bc
    ex de,hl
    ld hl,(mrSlot)
    call cmp_hl_de
    ex de,hl
    jp z,mrCeilingNext
    ld e,(hl)
    inc hl
    ld d,(hl)
    ld a,d
    or e
    jp z,mrCeilingNext
    ld hl,(mrCeiling)
    call cmp_hl_de
    jp c,mrCeilingNext
    ex de,hl
    ld (mrCeiling),hl
mrCeilingNext:
    pop bc
    pop hl
    ld de,4
    add hl,de
    dec b
    jp nz,mrCeilingLoop
    ld hl,(mrSize)
    ld a,h
    or l
    jp z,mrRelease
    ex de,hl
    ld hl,(mrCeiling)
    call cmp_hl_de
    jp c,memoryNoMem
    call sub_hl_de
    ld (mrBase),hl
    push hl
    ld hl,(mrFloor)
    ex de,hl
    pop hl
    call cmp_hl_de
    jp c,memoryNoMem
    ld a,h
    or a
    jp z,memoryNoMem
    ; No allocated MAIN object may cross the proposed resident boundary.
    ld hl,MEM_ALLOC_TABLE
    ld b,MEM_ALLOC_RECORDS
mrCheckBlock:
    ld a,(hl)
    or a
    jp nz,mrCheckNext
    push hl
    inc hl
    ld e,(hl)
    inc hl
    ld d,(hl)
    inc hl
    ld a,(hl)
    inc hl
    ld h,(hl)
    ld l,a
    add hl,de
    ex de,hl
    ld hl,(mrBase)
    call cmp_hl_de
    pop hl
    jp c,memoryDenied
mrCheckNext:
    ld de,6
    add hl,de
    dec b
    jp nz,mrCheckBlock
    ld hl,(mrBase)
    ld (mrCeiling),hl
    ld de,APP_MEMORY_END
    call mdReuseInvalidate
    jp mrCommit
mrRelease:
    ld hl,0
    ld (mrBase),hl
mrCommit:
    ld hl,(mrBase)
    ex de,hl
    ld hl,(mrSlot)
    ld (hl),e
    inc hl
    ld (hl),d
    inc hl
    push hl
    ld hl,(mrSize)
    ex de,hl
    pop hl
    ld (hl),e
    inc hl
    ld (hl),d
    ld hl,(mrCeiling)
    ld (memorySystemEnd),hl
    call memoryPublishTop
mrResult:
    ld hl,(mrSlot)
    ld e,(hl)
    inc hl
    ld d,(hl)
    inc hl
    push de
    ld e,(hl)
    inc hl
    ld d,(hl)
    ld hl,(memorySystemEnd)
    call memoryMainCeiling
    dec hl
    ld b,h
    ld c,l
    pop hl
    xor a
    ret
; Clamp a restored ancestor arena as well as the currently registered arena.
; HL=end -> min(end, system end). BC preserved; AF/DE destroyed.
memoryCapMain:
    ex de,hl
    ld hl,(memorySystemEnd)
    call cmp_hl_de
    ret c
    ex de,hl
    ret
memoryPublishTop:
    ld hl,(memorySystemEnd)
    call memoryMainCeiling
    dec hl
    xor a
    MXOS_CALL MXOS_BIOS_SET_MEM_TOP
    ret
; HL=system ceiling -> conservative contiguous MAIN end below loaded DLLs.
; Explicit FAR arenas can still use free gaps, checked against allocations.
; BC/DE preserved. MAIN applications respecting memTop cannot hit DLL BSS.
memoryMainCeiling:
    push bc
    push de
    ld (memoryPublishedEnd),hl
    ld hl,MODULE_TABLE
    ld b,MODULE_MAX
memoryCeilingModule:
    ld a,(hl)
    inc hl
    or (hl)
    dec hl
    jp z,memoryCeilingNext
    push hl
    ld de,3
    add hl,de
    ld a,(hl)
    or a
    jp nz,memoryCeilingPop
    ld de,7
    add hl,de
    ld e,(hl)
    inc hl
    ld d,(hl)
    inc hl
    ld a,(hl)
    inc hl
    ld h,(hl)
    ld l,a
    push de
    push hl
    ld hl,(memoryMainStart)
    ex de,hl
    pop hl
    call cmp_hl_de
    pop de
    jp c,memoryCeilingPop
    jp z,memoryCeilingPop
    ld hl,(memoryPublishedEnd)
    call cmp_hl_de
    jp c,memoryCeilingPop
    ex de,hl
    ld (memoryPublishedEnd),hl
memoryCeilingPop:
    pop hl
memoryCeilingNext:
    ld de,MODULE_RECORD_SIZE
    add hl,de
    dec b
    jp nz,memoryCeilingModule
    ld hl,MEM_ALLOC_TABLE
    ld b,MEM_ALLOC_RECORDS
memoryCeilingBlock:
    ld a,(hl)
    or a
    jp nz,memoryCeilingBlockNext
    push hl
    ld de,5
    add hl,de
    ld a,(appOwner)
    or a
    jp z,memoryCeilingBlockUse
    cp (hl)
    jp z,memoryCeilingBlockPop
memoryCeilingBlockUse:
    pop hl
    push hl
    inc hl
    ld e,(hl)
    inc hl
    ld d,(hl)
    inc hl
    ld a,(hl)
    inc hl
    ld h,(hl)
    ld l,a
    add hl,de
    push de
    push hl
    ld hl,(memoryMainStart)
    ex de,hl
    pop hl
    call cmp_hl_de
    pop de
    jp c,memoryCeilingBlockPop
    jp z,memoryCeilingBlockPop
    ld hl,(memoryPublishedEnd)
    call cmp_hl_de
    jp c,memoryCeilingBlockPop
    ex de,hl
    ld (memoryPublishedEnd),hl
memoryCeilingBlockPop:
    pop hl
memoryCeilingBlockNext:
    ld de,6
    add hl,de
    dec b
    jp nz,memoryCeilingBlock
    ld hl,(memoryPublishedEnd)
    pop de
    pop bc
    ret
memoryPublishedEnd: DW APP_MEMORY_END
; Cold DOS start or Bootstrap restoration of the default upper engine only.
; Ordinary shell/app returns must never call this reset.
memoryResidentReset:
    ld hl,mrSlots
    ld b,MEM_RESIDENT_SLOTS*4
    xor a
mrResetLoop:
    ld (hl),a
    inc hl
    dec b
    jp nz,mrResetLoop
    ld hl,APP_MEMORY_END
    ld (memorySystemEnd),hl
    ret
memorySystemEnd: DW APP_MEMORY_END
mrSlots: DS MEM_RESIDENT_SLOTS*4,0
mrSlot: DW 0
mrSize: DW 0
mrFloor: DW 0
mrBase: DW 0
mrCeiling: DW 0
mrOperation: DB 0
