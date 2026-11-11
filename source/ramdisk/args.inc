; RAMDISK -P n: ограничение B: перед его первоначальным форматированием.
; Готовый том с иной геометрией и выделенные FAR-блоки не изменяются.
BankArguments:
    call argNext
    jp c,BankNoArguments
    ld a,b
    cp 2
    jp nz,BankArgumentError
    ld a,(de)
    cp '-'
    jp nz,BankArgumentError
    inc de
    ld a,(de)
    MXOS_CALL MXOS_BIOS_UPPERCASE
    cp 'P'
    jp nz,BankArgumentError
    call argNext
    jp c,BankArgumentError
    ld a,b
    cp 3
    jp nc,BankArgumentError
    ld c,0
BankPageDigits:
    ld a,(de)
    sub '0'
    cp 10
    jp nc,BankArgumentError
    push af
    ld a,c
    add a,a
    add a,a
    add a,c
    add a,a
    ld c,a
    pop af
    add a,c
    cp 16
    jp nc,BankArgumentError
    ld c,a
    inc de
    dec b
    jp nz,BankPageDigits
    push bc
    call argNext
    pop bc
    jp nc,BankArgumentError
    push bc
    call mx_memInfo
    ld a,b
    pop bc
    dec a                 ; Только банк DOS; служба находится внутри него.
    jp c,BankArgumentError
    cp c
    jp nc,BankPagesAvailable
    ld c,a
BankPagesAvailable:
    ld a,c
    or a
    jp z,BankArgumentError
    ld b,c
    ld a,MEM_OP_RAMLIMIT
    MXOS_CALL MXOS_DOS_MEMORY
    ret nc
BankArgumentError:
    ld hl,bankArgumentText
    MXOS_CALL MXOS_BIOS_PRINT_STRING
    scf
    ret
BankNoArguments:
    ; Без аргументов первый запуск использует четыре банка, повтор сохраняет пул.
    ld c,MEM_RAMDISK_STATUS
    ld a,MEM_OP_RAMDISK
    MXOS_CALL MXOS_DOS_MEMORY
    ret c
    ld a,c
    or a
    ret nz
    ld hl,bankDefaultArgs
    jp BankArguments
bankDefaultArgs: DB '-P 4',0
bankArgumentText: DB 'RAMDISK: INVALID -P OR RAM DISK IS IN USE',0Dh,0Ah,0

InstallFailed:
    ld hl,txtNoRAM
    MXOS_CALL MXOS_BIOS_PRINT_STRING
    scf
    ret
txtNoRAM: DB 'RAMDISK: NO RAM DISK',0Ah,0
    INCLUDE "../common/cmp_hl_de.inc"
    INCLUDE "../common/argNext.inc"
