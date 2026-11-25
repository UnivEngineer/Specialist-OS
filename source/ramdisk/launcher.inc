;----------------------------------------------------------------------------
; MXOS
;
; Подготовить том B: и передать пул RAM-диска DOS через MEM_RAMDISK_COMMIT.
;
; вход:
;   de = указатель на строку аргументов ASCIIZ. Допустим ключ -P с числом
;   банков, см. BankArguments. Установка выполняется в банке DOS.
;
; выход:
;   cf = 0 после успешного COMMIT, cf = 1 при отказе. Если разбор аргументов
;   или установка вернули ошибку, COMMIT не вызывается. Регистры могут меняться.
;----------------------------------------------------------------------------
RamdiskLauncher:
    ex de,hl
    ld c,RAM_DRIVE
    call driverLetter
    jp c,BankArgumentError
    ld (bankRequestedDrive),a
    ld b,a
    push hl
    ld c,MEM_RAMDISK_CHECK_LETTER
    ld a,MEM_OP_RAMDISK
    MXOS_CALL_REGS MXOS_DOS_MEMORY
    pop hl
    jp c,BankArgumentError
    call BankArguments
    ret c
    call BankInstall
    ret c
    ld a,(bankRequestedDrive)
    ld b,a
    ld c,MEM_RAMDISK_COMMIT
    ld a,MEM_OP_RAMDISK
    MXOS_CALL_REGS MXOS_DOS_MEMORY
    ret

bankRequestedDrive: DB RAM_DRIVE
