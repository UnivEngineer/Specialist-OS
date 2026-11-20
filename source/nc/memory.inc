; Two packed catalogs share one allocation below the active font and memTop.
ncMemoryInit:
    call bios_getMemTop
    ld (ncMemoryTop),hl
    ld de,NC_MIN_END-1
    call bios_cmp_hl_de
    ret c
    inc hl
    ld de,FILE_CACHE_A
    call bios_sub_hl_de
    ld a,l
    and 0F0h
    ld l,a
    ld de,2*FILE_CACHE_BYTES
    call bios_cmp_hl_de
    jp c,ncMemorySizeReady
    ex de,hl
ncMemorySizeReady:
    ld (ncCatalogPoolSize),hl
    ex de,hl
    ld hl,FILE_CACHE_A
    add hl,de
    ld (ncMemoryEnd),hl
    ld hl,FILE_CACHE_A
    ld b,0
    ld a,MEM_OP_ARENA
    MXOS_CALL MXOS_DOS_MEMORY
    jp c,ncMemoryReserveError
    ld hl,(ncCatalogPoolSize)
    ld b,0
    ld a,MEM_OP_ALLOC
    MXOS_CALL MXOS_DOS_MEMORY
    ret nc
ncMemoryReserveError:
    push af
    ld hl,ncMemoryReserveMessage
    call bios_printStringZ
    pop af
    call bios_printHexByte
    jp ncMemoryRetryPrompt
ncCatalogPoolSize: DW 0
ncMemoryError:
    ld hl,ncMemoryMessage
    call bios_printStringZ
    ld hl,(ncMemoryTop)
    call bios_printHexWord
    ld hl,ncMemoryNeed
    call bios_printStringZ
    ld hl,NC_MIN_END-1
    call bios_printHexWord
ncMemoryRetryPrompt:
    ld hl,ncMemoryPrompt
    call bios_printStringZ
ncMemoryWait:
    call bios_getch
    cp 13
    jp nz,ncMemoryWait
    ld c,10
    call bios_printChar
    ; Avoid retrying NC forever when it cannot fit. CLI can install a smaller
    ; font module or release other resident resources before the next attempt.
    ld a,1
    MXOS_CALL MXOS_DOS_SET_SHELL
    jp bios_reboot
ncCatalogCapacity: DB FILE_CACHE_CAPACITY
ncMemoryTop: DW 0
ncMemoryEnd: DW NC_END
ncMemoryMessage: DB 'NC: not enough MAIN memory. MEMTOP=',0
ncMemoryNeed: DB ', need >= ',0
ncMemoryReserveMessage: DB 'NC: catalog allocation error ',0
ncMemoryPrompt: DB 13,10,'Press Enter for CLI.',13,10,0
