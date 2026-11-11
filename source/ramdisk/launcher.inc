; Одноразовый установщик: подготовить том и передать фиксированный пул DOS.
RamdiskLauncher:
    ex de,hl
    call BankArguments
    ret c
    call BankInstall
    ret c
    ld c,MEM_RAMDISK_COMMIT
    ld a,MEM_OP_RAMDISK
    MXOS_CALL MXOS_DOS_MEMORY
    ret
