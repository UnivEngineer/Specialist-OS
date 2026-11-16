; Scratch pages have fixed addresses. Three catalog tails share all remaining
; MAIN memory below memTop; each gets capacity records and a padded sentinel.
; No renderer calls or reservation changes: FONT/MEM own those resources.
ncMemoryInit:
    call bios_getMemTop
    ld (ncMemoryTop),hl
    ld de,NC_MIN_END-1
    call bios_cmp_hl_de
    ret c
    ex de,hl ; DE=inclusive memTop
    ld hl,FILE_CACHE_A+3*FILE_INFO_SIZE-1
    ld b,0
ncMemoryCapacity:
    push de
    ld de,3*FILE_INFO_SIZE
    add hl,de
    pop de
    call bios_cmp_hl_de
    jp c,ncMemoryFits
    jp nz,ncMemoryReady
ncMemoryFits:
    inc b
    ld a,b
    cp FILE_CACHE_CAPACITY
    jp c,ncMemoryCapacity
ncMemoryReady:
    ld a,b
    ld (ncCatalogCapacity),a
    ld l,b
    ld h,0
    inc hl
    DUP 4
        add hl,hl
    EDUP
    ex de,hl ; DE=per-catalog stride, including sentinel/padding
    ld hl,FILE_CACHE_A
    ld (ncCatalogBases),hl
    add hl,de
    ld (ncCatalogBases+2),hl
    add hl,de
    ld (ncCatalogBases+4),hl
    add hl,de
    ld (ncMemoryEnd),hl
    xor a
    ret
ncMemoryError:
    ld hl,ncMemoryMessage
    call bios_printStringZ
    ld hl,(ncMemoryTop)
    call bios_printHexWord
    ld hl,ncMemoryNeed
    call bios_printStringZ
    ld hl,NC_MIN_END-1
    call bios_printHexWord
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
ncMemoryMessage: DB 13,10,'NC: not enough MAIN memory. MEMTOP=',0
ncMemoryNeed: DB ', need >= ',0
ncMemoryPrompt: DB 13,10,'Press Enter for CLI.',13,10,0
