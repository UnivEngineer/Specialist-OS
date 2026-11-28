; Two packed catalogs share one allocation below the active font and memTop.
ncMemoryInit:
    call mx_getMemTop
    ld (ncMemoryTop),hl
    ld de,NC_MIN_END-1
    call mon2_cmp_hl_de
    ret c
    inc hl
    ld de,FILE_CACHE_A
    call nativeMath.sub_hl_de
    ld a,l
    and 0F0h
    ld l,a
    ld de,2*FILE_CACHE_BYTES
    call mon2_cmp_hl_de
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
    MXOS_CALL_REGS MXOS_DOS_MEMORY
    jp c,ncMemoryReserveError
    ld hl,(ncCatalogPoolSize)
    ld b,0
    ld a,MEM_OP_ALLOC
    MXOS_CALL_REGS MXOS_DOS_MEMORY
    ret nc
ncMemoryReserveError:
    push af
    ld hl,ncMemoryReserveMessage
    call mon2_printStringOld
    pop af
    call mx_printHexByte
    jp ncMemoryRetryPrompt
ncCatalogPoolSize: DW 0
ncMemoryError:
    ld hl,ncMemoryMessage
    call mon2_printStringOld
    ld hl,(ncMemoryTop)
    call mx_printHexWord
    ld hl,ncMemoryNeed
    call mon2_printStringOld
    ld hl,NC_MIN_END-1
    call mx_printHexWord
ncMemoryRetryPrompt:
    ld hl,ncMemoryPrompt
    call mon2_printStringOld
ncMemoryWait:
    call mon2_getchOld
    cp 13
    jp nz,ncMemoryWait
    ld c,10
    call mon2_printCharOld
    ; Avoid retrying NC forever when it cannot fit. CLI can install a smaller
    ; font module or release other resident resources before the next attempt.
    ld a,1
    MXOS_CALL_REGS MXOS_DOS_SET_SHELL
    jp mon2_init
ncCatalogCapacity:          DB FILE_CACHE_CAPACITY
ncMemoryTop:                DW 0
ncMemoryEnd:                DW NC_END
ncMemoryMessage:            DB 'NC: not enough MAIN memory. MEMTOP=',0
ncMemoryNeed:               DB ', need >= ',0
ncMemoryReserveMessage:     DB 'NC: catalog allocation error ',0
ncMemoryPrompt:             DB 13,10,'Press Enter for CLI.',13,10,0
